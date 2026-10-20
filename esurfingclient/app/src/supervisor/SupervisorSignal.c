#include "SupervisorInternal.h"

/* ------------------------------------------------------------------
 * 信号处理
 *
 * 处理函数里只置标志, 真正的关闭动作放在主循环里做 ——
 * 在信号处理函数里 join / kill / 打日志都不是 async-signal-safe 的
 * ------------------------------------------------------------------ */

#ifndef _WIN32

static void supervisor_signal_handler(const int sig)
{
    (void)sig;
    g_supervisor_stop = 1;
}

#else

static BOOL WINAPI supervisor_console_handler(const DWORD ctrl_type)
{
    (void)ctrl_type;
    g_supervisor_stop = 1;
    return TRUE;
}

#endif

void supervisor_install_signals(void)
{
#ifndef _WIN32
    signal(SIGTERM, supervisor_signal_handler);
    signal(SIGINT, supervisor_signal_handler);
    signal(SIGHUP, supervisor_signal_handler);
    signal(SIGQUIT, supervisor_signal_handler);
    // 回收靠主循环里的 waitpid, 不需要 SIGCHLD 处理函数
    signal(SIGCHLD, SIG_DFL);
#else
    SetConsoleCtrlHandler(supervisor_console_handler, TRUE);
#endif
}
