#include "supervisor/Supervisor.h"

#include "config/Config.h"

#include "control/Control.h"

#include "core/States.h"

#include "dialer/DialerClient.h"

#include "utils/log/Logger.h"
#include "utils/text/TextUtils.h"
#include "utils/time/TimeUtils.h"

#include "SupervisorInternal.h"

#define SUPERVISOR_CFG_CHECK_MS 1000

/** 找控制通道端口时最多往后看多少个 (正常情况下第一个就可用) */
#define SUPERVISOR_PORT_SCAN 64

static bool s_cfg_usable = false;

static uint64_t s_cfg_hash = 0;

static uint64_t s_cfg_check_time = 0;

/* ------------------------------------------------------------------
 * 回收与重启
 * ------------------------------------------------------------------ */

#ifndef _WIN32

static void supervisor_reap(void)
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

static void supervisor_reap(void)
{
    for (int i = 0; i < g_child_count; i++)
    {
        child_t* child = &g_children[i];
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

/**
 * @brief 这个子进程现在能不能拉起来
 *
 * 配置不可用时认证子进程拉起来也是白拉 (它自己会退出), 留着让用户先把配置改好。
 * 账号还可能在网页上被改过, 所以拉起之前按当前配置对一次号 ——
 * 拿一个已经不存在的序号去拉, 子进程只会反复退出
 * @param child 子进程
 * @param index 该子进程在表里的下标
 * @return 是否可以拉起
 */
static bool supervisor_child_ready(child_t* child, const int index)
{
    if (child->kind != CHILD_AUTH) return true;
    if (s_cfg_usable == false) return false;

    child->account = (index < g_prog_cnt) ? g_prog_status[index].cfg.idx : 0;

    return child->account != 0;
}

static void supervisor_restart_due(void)
{
    const uint64_t now = get_cur_tm_ms();

    for (int i = 0; i < g_child_count; i++)
    {
        child_t* child = &g_children[i];
        if (child->running) continue;
        if (now < child->next_start_time) continue;
        if (supervisor_child_ready(child, i) == false) continue;

        char name[32];
        child_name(child, name, sizeof(name));

        if (child_spawn(child))
        {
            LOG_DEBUG("%s 已重新拉起", name);
        }
    }
}

/**
 * @brief 按配置文件的变化调整子进程
 *
 * 监管者与子进程之间没有通道, 网页上的"保存并应用"传不到这里, 只能盯着配置文件:
 * 内容变了才重新解析一次 (解析一次要写好几行日志, 每 200 毫秒来一次会把日志刷满)。
 *
 * 配置从不可用变回可用时要主动把认证子进程拉起来 —— 它们没有"待机"的形态,
 * 不可用期间压根没被拉起来过
 */
static void supervisor_sync_cfg(void)
{
    // 测间隔用单调钟: 墙钟被 NTP 校时跳一下, 这里就会白等 (或立刻重来)
    const uint64_t now = get_steady_tm_ms();
    if (now - s_cfg_check_time < SUPERVISOR_CFG_CHECK_MS) return;
    s_cfg_check_time = now;

    const uint64_t hash = cfg_file_hash();
    if (hash == s_cfg_hash) return;
    s_cfg_hash = hash;

    const bool usable = load_cfg();

    if (usable == s_cfg_usable) return;
    s_cfg_usable = usable;

    if (usable)
    {
        LOG_DEBUG("配置已可用, 认证进程开始拉起");

        // 退避清零: 之前拉不起来是因为配置不可用, 不该按失败次数继续等下去
        for (int i = 0; i < g_child_count; i++)
        {
            if (g_children[i].kind != CHILD_AUTH) continue;
            g_children[i].restarts = 0;
            g_children[i].next_start_time = 0;
        }
    }
    else
    {
        LOG_WARN("配置已不可用, 认证进程不再拉起 (网页仍可访问)");
    }
}

/**
 * @brief 有序关闭所有子进程
 */
static void supervisor_shutdown(void)
{
    LOG_DEBUG("守护进程正在停止子进程");

    /**
     * 关窗口/注销/关机时系统只给几秒, 而认证进程退出前还要发一次登出请求
     * (网络请求)。这里不能像平常那样等 15 秒 —— 等太久监管进程会先被系统杀掉,
     * run.log 就来不及改名归档。登出没做完由会话存档在下次启动时补上。
     */
    const uint32_t web_ms = (g_console_closed != 0) ? SUPERVISOR_STOP_CONSOLE_MS : SUPERVISOR_STOP_WEB_MS;
    const uint32_t auth_ms = (g_console_closed != 0) ? SUPERVISOR_STOP_CONSOLE_MS : SUPERVISOR_STOP_AUTH_MS;

    // 先停 Web: 不再接受新的控制请求, 它也没有需要收尾的会话
    for (int i = 0; i < g_child_count; i++)
    {
        if (g_children[i].kind == CHILD_WEB)
        {
            child_stop(&g_children[i], web_ms);
        }
    }

    /**
     * 再停认证进程: 它们退出前会走一遍登出流程 (term() 是网络请求),
     * 因此给的等待时间要长一些, 不够再强杀
     */
    for (int i = 0; i < g_child_count; i++)
    {
        if (g_children[i].kind == CHILD_AUTH)
        {
            child_stop(&g_children[i], auth_ms);
        }
    }

    LOG_DEBUG("守护进程已停止全部子进程");
}

/* ------------------------------------------------------------------
 * 主流程
 * ------------------------------------------------------------------ */

/**
 * @brief 把任意端口号折回 1..65535
 *
 * 0 在 bind 里的意思是"让内核随便挑一个", 传 0 出去监管者自己却仍按 0 去连,
 * 必然连不上, 所以不能出现在端口号里。
 */
static uint16_t port_wrap(const uint32_t port)
{
    return (uint16_t)(((port - 1u) % 65535u) + 1u);
}

/**
 * @brief 挑一个可用的控制通道起始端口
 *
 * 需要 base 到 base + count - 1 全部可绑定: 每个认证子进程占一个端口
 * (见 child_control_port), Web 子进程连 base。
 *
 * @param base 默认起始端口
 * @param count 认证子进程数 (配置不可用时传 <= 0, 按 1 个算)
 * @return 可用的起始端口; 全都被占时返回 base, 让子进程按原来的降级逻辑处理
 */
static uint16_t supervisor_pick_control_port(const uint16_t base, const int8_t count)
{
    const uint8_t need = (count > 0) ? (uint8_t)count : 1;

    for (uint16_t offset = 0; offset < SUPERVISOR_PORT_SCAN; offset++)
    {
        const uint16_t candidate = port_wrap((uint32_t)base + offset);
        bool usable = true;

        for (uint8_t i = 0; i < need; i++)
        {
            if (control_port_available(port_wrap((uint32_t)candidate + i)) == false)
            {
                usable = false;
                break;
            }
        }

        if (usable) return candidate;
    }

    return base;
}

/**
 * @brief 按配置里的可用账号构建子进程表
 *
 * 认证子进程要排在 Web 子进程前面: 控制端口是按表里的顺序分配的 (第 i 个认证进程用
 * 端口 + i), 而 Web 进程连的是第 0 个认证进程的端口 —— 顺序反了它就连不上认证进程了
 * @return 是否构建成功
 */
static bool supervisor_build_children(void)
{
    g_child_count = 0;

    /**
     * 一个账号都没有时也留一个认证进程的位置: 用户在网页上把账号填好之后
     * 就能直接把它拉起来, 不必重启服务
     */
    const uint8_t auth_slots = (g_prog_cnt > 0) ? (uint8_t)g_prog_cnt : 1;

    for (uint8_t i = 0; i < auth_slots && g_child_count < SUPERVISOR_MAX_CHILDREN; i++)
    {
        child_t* child = &g_children[g_child_count];
        memset(child, 0, sizeof(*child));
        child->handle = CHILD_HANDLE_INVALID;
        child->kind = CHILD_AUTH;
        child->account = (i < g_prog_cnt) ? g_prog_status[i].cfg.idx : 0;
        child->control_port = child_control_port(CHILD_AUTH, g_child_count);
        g_child_count++;
    }

    if (g_child_count >= SUPERVISOR_MAX_CHILDREN)
    {
        LOG_WARN("账号数超过上限 (%d), 多余的账号不会启动", SUPERVISOR_MAX_CHILDREN - 1);
        return g_child_count > 0;
    }

    child_t* web_child = &g_children[g_child_count];
    memset(web_child, 0, sizeof(*web_child));
    web_child->handle = CHILD_HANDLE_INVALID;
    web_child->kind = CHILD_WEB;
    web_child->control_port = child_control_port(CHILD_WEB, g_child_count);
    g_child_count++;

    return true;
}

int work_supervisor(void)
{
    if (app_bootstrap() == false) return 1;

    LOG_DEBUG(" - 以守护进程运行: 认证与 Web 各起独立进程");

    /**
     * 配置不可用时仍然要起来 (启动 Web 子进程), 所以这里不返回失败:
     * 网页正是用户把配置改回来的入口, 而服务模式下没有网页就等于没有出路。
     *
     * 配置有问题时监管者会被反复重启 (systemd/procd/SCM 的重启策略),
     * 失败路径刻意【不】调 clean_logger(): 它会做两件事 —— 关句柄 (exit() 本来也会关)
     * 和把 run.log 改名归档, 每次都归档就会在日志目录里堆一堆只有横幅的垃圾文件
     */
    s_cfg_usable = load_cfg();
    s_cfg_hash = cfg_file_hash();

    if (s_cfg_usable == false)
    {
        LOG_WARN("配置暂不可用, 先只启动 Web 进程, 请在网页界面上改好配置");
        LOG_INFO("配置文件路径: %s", get_config_path());
    }

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

    /**
     * 控制通道端口: 默认端口被别的程序占着时自动往后顺延
     *
     * 每个认证子进程占一个端口 (base + 下标), Web 子进程连 base。默认的 8890
     * 在本机就遇到过被系统进程 (dllhost) 监听, 结果认证进程绑不上、远程控制整体
     * 降级 —— 网页显示"认证进程不可达", 重启只能强杀。所以这里先探一遍再下发。
     *
     * Web 服务端口 (配置里的 web_port) 不在此列: 那是用户明确指定的, 冲突就报错。
     */
    const uint16_t picked_port = supervisor_pick_control_port(g_control_port, g_prog_cnt);
    if (picked_port != g_control_port)
    {
        LOG_WARN("控制通道端口 %" PRIu16 " 已被占用, 改用 %" PRIu16, g_control_port, picked_port);
        g_control_port = picked_port;
    }

    if (supervisor_build_children() == false) return 1;

    supervisor_install_signals();

    int started = 0;
    for (int i = 0; i < g_child_count; i++)
    {
        // 配置不可用时只拉 Web 子进程, 认证子进程等配置可用了再补
        if (supervisor_child_ready(&g_children[i], i) == false) continue;

        char name[32];
        if (child_spawn(&g_children[i]))
        {
            LOG_DEBUG("已启动 %s, PID %lu", child_name(&g_children[i], name, sizeof(name)),
                child_pid(&g_children[i]));
            started++;
        }
        else
        {
            LOG_ERROR("启动 %s 失败", child_name(&g_children[i], name, sizeof(name)));
        }
    }

    if (started == 0)
    {
        LOG_FATAL("没有任何子进程启动成功");
        return 1;
    }

    if (s_cfg_usable)
    {
        LOG_DEBUG("守护进程已就绪, 共 %d 个子进程 (认证 %" PRId8 " 个 + Web 1 个)", started, g_prog_cnt);
    }
    else
    {
        LOG_DEBUG("守护进程已就绪, 当前只有 Web 进程; 配置可用后会自动补上认证进程");
    }

    /**
     * 主循环: 回收退出的子进程 -> 到点重新拉起 -> 看配置有没有变过 -> 睡一小会儿
     *
     * 两个退出条件:
     * - g_supervisor_stop: 自己装的信号处理 (POSIX 的 SIGTERM 等)
     * - g_need_exit: 其它路径请求关闭 (Windows 服务收到 SCM 停止请求时走 shut())
     *
     * 睡眠用可打断版本, 这样收到信号能及时进入关闭流程
     */
    while (g_supervisor_stop == 0 && g_need_exit == false)
    {
        supervisor_reap();
        supervisor_restart_due();
        supervisor_sync_cfg();
        sleep_ms(SUPERVISOR_TICK_MS, true);
    }

    LOG_DEBUG("守护进程收到退出请求");
    supervisor_shutdown();

    // 监管者是 run.log 收尾改名的那一方 (认证/Web 子进程都会跳过改名)
    clean_logger();
    g_shutdown_done = 1; // 让控制台处理线程尽快返回, 别让窗口卡着不走
    return 0;
}
