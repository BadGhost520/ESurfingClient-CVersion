#include "control/Control.h"

#include "states/States.h"

#include "utils/PlatformUtils.h"
#include "utils/Logger.h"

#include "supervisor/SupervisorInternal.h"

/* ------------------------------------------------------------------
 * 子进程名称 (日志用)
 * ------------------------------------------------------------------ */

const char* child_name(const child_t* child, char* buf, const size_t len)
{
    if (child->kind == CHILD_WEB)
    {
        snprintf(buf, len, "Web 进程");
    }
    else
    {
        snprintf(buf, len, "认证进程(配置 %" PRIu8 ")", child->account);
    }
    return buf;
}

/**
 * @brief 取子进程的进程号 (仅用于日志)
 *
 * Windows 侧句柄是指针, 不能直接往整数里塞 (64 位下尺寸不一样), 要问系统要 PID
 * @param child 子进程
 * @return 进程号, 未运行时为 0
 */
unsigned long child_pid(const child_t* child)
{
    if (child->running == false) return 0;

#ifdef _WIN32
    return (unsigned long)GetProcessId(child->handle);
#else
    return (unsigned long)child->handle;
#endif
}

/* ------------------------------------------------------------------
 * 拉起子进程
 * ------------------------------------------------------------------ */

/**
 * @brief 组装子进程的参数
 * @param child 子进程
 * @param exec_path 自身可执行文件路径
 * @param account_arg 配置序号文本缓冲
 * @param control_arg 控制端口文本缓冲
 * @param argv 输出参数数组 (至少 12 个元素)
 */
static void child_build_argv(const child_t* child, char* exec_path,
                             char* account_arg, char* control_arg, char** argv)
{
    snprintf(account_arg, 8, "%" PRIu8, child->account);
    snprintf(control_arg, 8, "%" PRIu16, child->control_port);

    int n = 0;
    argv[n++] = exec_path;

    if (child->kind == CHILD_AUTH)
    {
        argv[n++] = "--role";
        argv[n++] = "auth";
        argv[n++] = "--account";
        argv[n++] = account_arg;
    }
    else
    {
        argv[n++] = "--role";
        argv[n++] = "web";
    }

    // 控制端口由监管者统一决定, 原样传给子进程
    argv[n++] = "--control-port";
    argv[n++] = control_arg;

#ifdef _WIN32
    /**
     * 令牌由监管者生成, 认证与 Web 子进程必须拿到同一个。
     *
     * Windows 上只能走命令行: 想让 CreateProcessA 带上自定义环境变量,
     * 得自己拼一整块环境块并与现有环境合并, 而本机没有 Windows 可验证。
     * POSIX 上改用环境变量下发, 原因见 child_build_env()
     */
    if (g_control_token[0] != '\0')
    {
        argv[n++] = "--control-token";
        argv[n++] = g_control_token;
    }
#endif

    argv[n] = NULL;
}

#ifndef _WIN32

extern char** environ;

/**
 * @brief 组装子进程的环境变量, 把控制通道令牌塞进去
 *
 * 令牌不走命令行是有原因的: Linux 上 /proc/<PID>/cmdline 是**全局可读**的,
 * 同机其它用户 ps 一下就能拿到令牌。环境变量对应的 /proc/<PID>/environ
 * 只有属主和 root 可读, 能把暴露面收窄一个数量级。
 *
 * 新环境数组必须在 fork **之前**准备好: setenv() 内部会 malloc,
 * 而 fork 之后只允许调用 async-signal-safe 的函数 ——
 * 与上面"子进程里不能打日志"是同一个原因。
 *
 * @param token_env 存放 "ESURFING_CONTROL_TOKEN=<令牌>" 的缓冲
 * @param token_env_len 缓冲长度
 * @return 环境数组 (调用方负责 free); 没有令牌或分配失败时返回 NULL,
 *         此时调用方退回继承当前环境
 */
static char** child_build_env(char* token_env, const size_t token_env_len)
{
    if (g_control_token[0] == '\0') return NULL;

    snprintf(token_env, token_env_len, "ESURFING_CONTROL_TOKEN=%s", g_control_token);

    size_t count = 0;
    while (environ[count] != NULL) count++;

    // 多一个装令牌, 多一个放结尾的 NULL
    char** envp = malloc((count + 2) * sizeof(char*));
    if (envp == NULL)
    {
        LOG_WARN("环境数组分配失败, 令牌改走继承的环境 (子进程可能拿不到令牌)");
        return NULL;
    }

    for (size_t i = 0; i < count; i++) envp[i] = environ[i];
    envp[count] = token_env;
    envp[count + 1] = NULL;
    return envp;
}

bool child_spawn(child_t* child)
{
    char exec_path[PATH_MAX];
    if (get_exec_path(exec_path) == false)
    {
        LOG_FATAL("无法获取自身可执行文件路径, 不能拉起子进程");
        return false;
    }

    char account_arg[8];
    char control_arg[8];
    char* argv[12];
    child_build_argv(child, exec_path, account_arg, control_arg, argv);

    // 令牌走环境变量下发 (见 child_build_env 的说明)
    char token_env[CONTROL_TOKEN_LEN + 32];
    char** envp = child_build_env(token_env, sizeof(token_env));
    char** child_env = (envp != NULL) ? envp : environ;

    const pid_t pid = fork();
    if (pid < 0)
    {
        LOG_ERROR("fork 失败: %s", strerror(errno));
        free(envp);
        return false;
    }

    if (pid == 0)
    {
        /**
         * 子进程: 这里只能做 exec 前必须的事。
         * 不能打日志 —— vsnprintf 内部会 malloc, 在 fork 之后并不安全
         */
        signal(SIGTERM, SIG_DFL);
        signal(SIGINT, SIG_DFL);
        signal(SIGHUP, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGCHLD, SIG_DFL);

#if defined(__linux__)
        // 监管者被强杀时子进程跟着退出, 不留孤儿
        prctl(PR_SET_PDEATHSIG, SIGTERM);
#endif

        execve(exec_path, argv, child_env);
        _exit(127);
    }

    free(envp);

    child->handle = pid;
    child->running = true;
    child->start_time = get_cur_tm_ms();
    return true;
}

#else

bool child_spawn(child_t* child)
{
    char exec_path[PATH_MAX];
    if (get_exec_path(exec_path) == false)
    {
        LOG_FATAL("无法获取自身可执行文件路径, 不能拉起子进程");
        return false;
    }

    char account_arg[8];
    char control_arg[8];
    char* argv[12];
    child_build_argv(child, exec_path, account_arg, control_arg, argv);

    // Windows 需要一整条命令行字符串, 可执行文件路径可能含空格, 必须加引号
    char cmdline[PATH_MAX + 64];
    int used = snprintf(cmdline, sizeof(cmdline), "\"%s\"", exec_path);
    for (int i = 1; argv[i] != NULL && used > 0 && (size_t)used < sizeof(cmdline); i++)
    {
        used += snprintf(cmdline + used, sizeof(cmdline) - (size_t)used, " %s", argv[i]);
    }

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    memset(&si, 0, sizeof(si));
    memset(&pi, 0, sizeof(pi));
    si.cb = sizeof(si);

    /**
     * CREATE_NEW_PROCESS_GROUP 是必须的:
     * 子进程要单独成组, 下面 child_stop() 里的
     * GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT) 才有办法把"停止"送到它。
     * 否则 Windows 上只能硬杀, 认证子进程就没有时间跑完登出
     *
     * 不继承句柄: 子进程的日志由它自己打开
     */
    if (CreateProcessA(exec_path, cmdline, NULL, NULL, FALSE,
                       CREATE_NEW_PROCESS_GROUP, NULL, NULL, &si, &pi) == 0)
    {
        LOG_ERROR("CreateProcess 失败 (错误码 %lu)", (unsigned long)GetLastError());
        return false;
    }

    CloseHandle(pi.hThread);
    child->handle = pi.hProcess;
    child->running = true;
    child->start_time = get_cur_tm_ms();
    return true;
}

#endif

/**
 * @brief 安排子进程重启 (指数退避)
 * @param child 子进程
 * @param exit_code 退出码 (仅用于日志)
 */
void child_schedule_restart(child_t* child, const int exit_code)
{
    char name[32];
    child_name(child, name, sizeof(name));

    const uint64_t now = get_cur_tm_ms();

    // 活得够久说明不是启动就崩, 退避与计数清零
    if (now - child->start_time >= SUPERVISOR_HEALTHY_MS)
    {
        child->restarts = 0;
    }
    child->restarts++;

    if (child->restarts <= 1)
    {
        LOG_WARN("%s 已退出 (退出码 %d), 将重新拉起", name, exit_code);
    }
    else
    {
        LOG_ERROR("%s 已退出 (退出码 %d), 这是连续第 %" PRIu32 " 次", name, exit_code, child->restarts);
    }

    /**
     * 指数退避: 子进程反复秒退时不能无脑重拉, 否则就是 fork 风暴 + 刷屏。
     * 退避上限 1 分钟, 之后一直重试 —— 用户把配置改对了就能自动恢复
     */
    uint64_t delay = SUPERVISOR_BACKOFF_MIN_MS;
    for (uint32_t i = 1; i < child->restarts; i++)
    {
        if (delay >= SUPERVISOR_BACKOFF_MAX_MS) break;
        delay *= 2;
    }
    if (delay > SUPERVISOR_BACKOFF_MAX_MS) delay = SUPERVISOR_BACKOFF_MAX_MS;

    child->next_start_time = now + delay;
    LOG_INFO("%s 将在 %" PRIu64 " 毫秒后重新拉起", name, delay);
}

/* ------------------------------------------------------------------
 * 停止子进程
 * ------------------------------------------------------------------ */

#ifndef _WIN32

child_t* child_find(const child_handle_t handle)
{
    for (int i = 0; i < s_child_count; i++)
    {
        if (s_children[i].running && s_children[i].handle == handle)
        {
            return &s_children[i];
        }
    }
    return NULL;
}

/**
 * @brief 等待子进程退出
 * @param handle 进程句柄
 * @param timeout_ms 超时
 * @return 是否已退出
 */
static bool child_wait_exit(const child_handle_t handle, const uint32_t timeout_ms)
{
    uint32_t waited = 0;

    while (waited < timeout_ms)
    {
        int status = 0;
        const pid_t result = waitpid(handle, &status, WNOHANG);
        if (result == handle) return true;
        if (result < 0 && errno != EINTR) return true; // 已经被回收过了

        sleep_ms(SUPERVISOR_WAIT_SLICE_MS, false);
        waited += SUPERVISOR_WAIT_SLICE_MS;
    }

    return false;
}

void child_stop(child_t* child, const uint32_t grace_ms)
{
    if (child->running == false) return;

    char name[32];
    child_name(child, name, sizeof(name));

    LOG_INFO("正在停止 %s ...", name);

    if (kill(child->handle, SIGTERM) != 0)
    {
        LOG_WARN("向 %s 发送 SIGTERM 失败: %s", name, strerror(errno));
    }

    if (child_wait_exit(child->handle, grace_ms) == false)
    {
        LOG_WARN("%s 在 %" PRIu32 " 毫秒内没有退出, 强制结束", name, grace_ms);
        kill(child->handle, SIGKILL);

        int status = 0;
        waitpid(child->handle, &status, 0);
    }

    child->running = false;
    child->handle = CHILD_HANDLE_INVALID;

    LOG_INFO("%s 已停止", name);
}

#else

/**
 * @brief 把"请优雅退出"这个请求送到子进程
 *
 * 两条路, 按可靠性排序:
 *
 * 1. 控制通道 (首选)。它走回环 TCP, 不依赖控制台, 服务模式 / 控制台模式
 *    行为一致。认证子进程收到后置 g_stop_requested, 主循环退出、dialer_app
 *    跑完 clean() —— 登出就在这里发生。
 *
 * 2. 控制台事件 (退路)。只在前台运行时有效: 监管者和子进程共用一个控制台。
 *    **服务由 SCM 启动时没有控制台, 这个 API 必然失败** —— 这正是原来
 *    服务模式下子进程被强杀、登出不会发生的原因。
 *
 * @return 是否成功把请求送了出去 (送不出去时调用方不该干等)
 */
static bool child_ask_stop(const child_t* child, const char* name)
{
    if (child->kind == CHILD_AUTH)
    {
        control_set_port(child->control_port);
        if (control_request_shutdown())
        {
            LOG_DEBUG("已通过控制通道请求 %s 退出 (端口 %" PRIu16 ")", name, child->control_port);
            return true;
        }
    }

    if (GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, GetProcessId(child->handle)) != 0)
    {
        LOG_DEBUG("已向 %s 发送控制台事件", name);
        return true;
    }

    return false;
}

void child_stop(child_t* child, const uint32_t grace_ms)
{
    if (child->running == false) return;

    char name[32];
    child_name(child, name, sizeof(name));

    LOG_INFO("正在停止 %s ...", name);

    const bool asked = child_ask_stop(child, name);
    if (asked == false)
    {
        LOG_WARN("%s 既没有控制通道也不接收控制台事件, 无法请求优雅退出 (即将强制结束)", name);
    }

    // 请求都没送出去就不必等满宽限期: 它不可能自己退出
    const uint32_t wait_ms = asked ? grace_ms : SUPERVISOR_STOP_NO_GRACE_MS;

    if (WaitForSingleObject(child->handle, wait_ms) == WAIT_OBJECT_0)
    {
        CloseHandle(child->handle);
        child->handle = CHILD_HANDLE_INVALID;
        child->running = false;
        LOG_INFO("%s 已停止", name);
        return;
    }

    LOG_WARN("%s 在 %" PRIu32 " 毫秒内没有退出, 强制结束", name, wait_ms);
    TerminateProcess(child->handle, 1);
    WaitForSingleObject(child->handle, 5000);
    CloseHandle(child->handle);
    child->handle = CHILD_HANDLE_INVALID;
    child->running = false;

    LOG_INFO("%s 已停止", name);
}

#endif

/**
 * @brief 算出某个子进程该用哪个控制端口
 *
 * 每个认证进程一个端口: 它们都要开控制服务, 共用一个端口的话只有先绑上的那个
 * 能开成, 其余会打"控制通道启动失败"并降级为不可远程控制 —— 那样监管者也就
 * 没法请它们优雅退出了。
 * Web 进程不用开服务, 它连的是第一个认证进程的端口。
 * @param kind 子进程类型
 * @param index 该子进程在表里的下标
 * @return 端口
 */
uint16_t child_control_port(const child_kind_t kind, const int index)
{
    uint32_t port = (uint32_t)g_control_port + (kind == CHILD_AUTH ? (uint32_t)index : 0);

    /**
     * 端口号是 16 位, 账号特别多时可能越界, 所以回绕 —— 但要【避开 0】:
     * 0 在 bind 里的意思是"让内核随便挑一个", 传 0 出去监管者自己却仍按 0 去连,
     * 必然连不上, 那个子进程就只能被硬杀 (跑不到 clean(), 也就不会登出)。
     * 下面这个写法把任意值映射到 1..65535。
     */
    port = ((port - 1u) % 65535u) + 1u;

    return (uint16_t)port;
}
