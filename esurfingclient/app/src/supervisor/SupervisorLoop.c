#include "supervisor/Supervisor.h"

#include "states/States.h"

#include "utils/PlatformUtils.h"
#include "utils/Logger.h"

#include "supervisor/SupervisorInternal.h"

/* ------------------------------------------------------------------
 * 回收与重启
 * ------------------------------------------------------------------ */

#ifndef _WIN32

static void supervisor_reap()
{
    int status = 0;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        child_t* child = child_find(pid);
        if (child == NULL) continue;

        child->running = false;
        child->handle = CHILD_HANDLE_INVALID;

        int exit_code = -1;
        if (WIFEXITED(status)) exit_code = WEXITSTATUS(status);
        else if (WIFSIGNALED(status)) exit_code = 128 + WTERMSIG(status);

        child_schedule_restart(child, exit_code);
    }
}

#else

static void supervisor_reap()
{
    for (int i = 0; i < s_child_count; i++)
    {
        child_t* child = &s_children[i];
        if (child->running == false) continue;

        DWORD code = 0;
        if (GetExitCodeProcess(child->handle, &code) == 0) continue;
        if (code == STILL_ACTIVE) continue;

        CloseHandle(child->handle);
        child->handle = CHILD_HANDLE_INVALID;
        child->running = false;

        child_schedule_restart(child, (int)code);
    }
}

#endif

static void supervisor_restart_due()
{
    const uint64_t now = get_cur_tm_ms();

    for (int i = 0; i < s_child_count; i++)
    {
        child_t* child = &s_children[i];
        if (child->running) continue;
        if (now < child->next_start_time) continue;

        char name[32];
        child_name(child, name, sizeof(name));

        if (child_spawn(child))
        {
            LOG_INFO("%s 已重新拉起", name);
        }
    }
}

/**
 * @brief 有序关闭所有子进程
 */
static void supervisor_shutdown()
{
    LOG_INFO("守护进程正在停止子进程");

    // 先停 Web: 不再接受新的控制请求, 它也没有需要收尾的会话
    for (int i = 0; i < s_child_count; i++)
    {
        if (s_children[i].kind == CHILD_WEB)
        {
            child_stop(&s_children[i], SUPERVISOR_STOP_WEB_MS);
        }
    }

    /**
     * 再停认证进程: 它们退出前会走一遍登出流程 (term() 是网络请求),
     * 因此给的等待时间要长一些, 不够再强杀
     */
    for (int i = 0; i < s_child_count; i++)
    {
        if (s_children[i].kind == CHILD_AUTH)
        {
            child_stop(&s_children[i], SUPERVISOR_STOP_AUTH_MS);
        }
    }

    LOG_INFO("守护进程已停止全部子进程");
}

/* ------------------------------------------------------------------
 * 主流程
 * ------------------------------------------------------------------ */

/**
 * @brief 按配置里的可用账号构建子进程表
 * @return 是否至少构建出一个子进程
 */
static bool supervisor_build_children()
{
    if (g_prog_cnt <= 0)
    {
        LOG_FATAL("没有可用账号, 无法启动认证进程");
        return false;
    }

    s_child_count = 0;

    for (uint8_t i = 0; i < g_prog_cnt && s_child_count < SUPERVISOR_MAX_CHILDREN; i++)
    {
        child_t* child = &s_children[s_child_count];
        memset(child, 0, sizeof(*child));
        child->handle = CHILD_HANDLE_INVALID;
        child->kind = CHILD_AUTH;
        child->account = g_prog_status[i].login_cfg.idx;
        child->control_port = child_control_port(CHILD_AUTH, s_child_count);
        s_child_count++;
    }

    if (s_child_count >= SUPERVISOR_MAX_CHILDREN)
    {
        LOG_WARN("账号数超过上限 (%d), 多余的账号不会启动", SUPERVISOR_MAX_CHILDREN - 1);
        return s_child_count > 0;
    }

    child_t* web_child = &s_children[s_child_count];
    memset(web_child, 0, sizeof(*web_child));
    web_child->handle = CHILD_HANDLE_INVALID;
    web_child->kind = CHILD_WEB;
    web_child->control_port = child_control_port(CHILD_WEB, s_child_count);
    s_child_count++;

    return true;
}

int work_supervisor()
{
    g_thread_keep_alive = true;

    g_prog_status = calloc(1, sizeof(prog_status_t));

    if (init_logger() == false) return 1;

    LOG_INFO(" - 程序版本: " PROGRAM_FULL_VERSION);
    LOG_INFO(" - 以守护进程运行: 认证与 Web 各起独立进程");

    if (load_cfg() == false) return 1;

    /**
     * 这里的失败路径刻意【不】调 clean_logger()。
     *
     * 它会做两件事: 关句柄 (exit() 本来也会关) 和把 run.log 改名归档。
     * 而配置有问题时监管者会被反复重启 (systemd/procd/SCM 的重启策略),
     * 每次都归档就会在日志目录里堆一堆只有横幅的垃圾文件。
     * 不退化成重启循环的话, 下一次正常退出时的归档会把日志一起收走。
     */

    /**
     * 生成一次性令牌下发给子进程:
     * 控制通道只监听回环, 但本机其它进程同样连得上。有了令牌,
     * 无关进程就没法通过通道下发"重新认证 / 应用新配置"这类动作
     */
    unsigned char token_bytes[16];
    get_rand_bytes(token_bytes, sizeof(token_bytes));
    for (size_t i = 0; i < sizeof(token_bytes); i++)
    {
        snprintf(g_control_token + i * 2, 3, "%02x", token_bytes[i]);
    }

    if (supervisor_build_children() == false) return 1;

    supervisor_install_signals();

    int started = 0;
    for (int i = 0; i < s_child_count; i++)
    {
        char name[32];
        if (child_spawn(&s_children[i]))
        {
            LOG_INFO("已启动 %s, PID %lu", child_name(&s_children[i], name, sizeof(name)),
                child_pid(&s_children[i]));
            started++;
        }
        else
        {
            LOG_ERROR("启动 %s 失败", child_name(&s_children[i], name, sizeof(name)));
        }
    }

    if (started == 0)
    {
        LOG_FATAL("没有任何子进程启动成功");
        return 1;
    }

    LOG_INFO("守护进程已就绪, 共 %d 个子进程 (认证 %" PRId8 " 个 + Web 1 个)", started, g_prog_cnt);

    /**
     * 主循环: 回收退出的子进程 -> 到点重新拉起 -> 睡一小会儿
     *
     * 两个退出条件:
     * - s_stop_requested: 自己装的信号处理 (POSIX 的 SIGTERM 等)
     * - g_need_exit: 其它路径请求关闭 (Windows 服务收到 SCM 停止请求时走 shut())
     *
     * 睡眠用可打断版本, 这样收到信号能及时进入关闭流程
     */
    while (s_stop_requested == 0 && g_need_exit == false)
    {
        supervisor_reap();
        supervisor_restart_due();
        sleep_ms(SUPERVISOR_TICK_MS, true);
    }

    LOG_INFO("守护进程收到退出请求");
    supervisor_shutdown();

    // 监管者是 run.log 收尾改名的那一方 (认证/Web 子进程都会跳过改名)
    clean_logger();
    return 0;
}
