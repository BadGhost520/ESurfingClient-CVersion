#include "SupervisorInternal.h"

#include "core/States.h"

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
    g_supervisor_stop = 1;
    g_stop_requested = 1;

    /**
     * 关窗口 / 注销 / 关机: 系统只给几秒, 而我们还要停两个子进程、再把 run.log
     * 改名归档。所以在这里等主线程收尾 (最多约 4.5 秒), 完了再返回。
     *
     * 返回 TRUE 是刻意的: 通用处理函数 (Shutdown.c) 在链上后面, 让它再等一轮
     * 只会白耗时间, 直接把它挡掉。
     */
    if (ctrl_type == CTRL_CLOSE_EVENT || ctrl_type == CTRL_LOGOFF_EVENT || ctrl_type == CTRL_SHUTDOWN_EVENT)
    {
        g_console_closed = 1; // 主线程据此把等子进程的时间压短

        for (int i = 0; i < 450 && g_shutdown_done == 0; i++)
        {
            Sleep(10);
        }

        if (g_shutdown_done != 0) Sleep(300);

        return TRUE;
    }

    // Ctrl+C / Ctrl+Break: 只置标志, 由主循环走正常退出
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
