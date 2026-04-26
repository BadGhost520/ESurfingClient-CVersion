// #include "webserver/WebServer.h"
#include "utils/Shutdown.h"
#include "utils/Logger.h"
#include "States.h"

#include <signal.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#endif

void shut(const uint8_t exitCode)
{
    LOG_INFO("主程序正在关闭");
    // if (is_webserver_running) stopWebServer();
    LOG_INFO("关闭线程守护");
    thread_keep_alive = false;
    LOG_INFO("清理资源中");
    LOG_DEBUG("关闭线程");
    for (uint8_t i = 0; i < g_prog_cnt; i++)
    {
        int result_code = 0;
        g_prog_status[i].runtime_status.is_running = false;
        sim_thread_join(g_prog_status[i].thread, &result_code);
    }
    LOG_INFO("退出程序");
    clean_logger();
    exit(exitCode);
}

#ifdef _WIN32
// Windows 控制台事件处理
static BOOL WINAPI console_handler(const DWORD ctrlType)
{
    switch(ctrlType)
    {
    case CTRL_C_EVENT:
        LOG_DEBUG("接收到 CTRL+C 信号");
        shut(0);
        return TRUE;
    case CTRL_BREAK_EVENT:
        LOG_DEBUG("接收到 CTRL+BREAK 信号");
        shut(0);
        return TRUE;
    case CTRL_CLOSE_EVENT:
        LOG_DEBUG("接收到窗口关闭信号");
        shut(0);
        return TRUE;
    case CTRL_LOGOFF_EVENT:
        LOG_DEBUG("接收到用户注销信号");
        shut(0);
        return TRUE;
    case CTRL_SHUTDOWN_EVENT:
        LOG_DEBUG("接收到系统关机信号");
        shut(0);
        return TRUE;
    default:
        LOG_DEBUG("接收到未知控制台事件: %lu", ctrlType);
        return FALSE;
    }
}
#else
// Linux/Unix 信号处理
static void signal_handler(const int sig)
{
    switch(sig)
    {
    case SIGINT:
        LOG_DEBUG("接收到 SIGINT 信号 (Ctrl+C)");
        shut(0);
        break;
    case SIGTERM:
        LOG_DEBUG("接收到 SIGTERM 信号 (Terminate request)");
        shut(0);
        break;
    case SIGHUP:
        LOG_DEBUG("接收到 SIGHUP 信号 (终端断开)");
        shut(0);
        break;
    case SIGQUIT:
        LOG_DEBUG("接收到 SIGQUIT 信号 (Quit request)");
        shut(0);
        break;
    default:
        LOG_DEBUG("接收到未处理的信号: %d", sig);
        shut(0);
    }
}
#endif

void init_shutdown_hook()
{
#ifdef _WIN32
    // Windows: 使用控制台事件处理
    if (SetConsoleCtrlHandler(console_handler, TRUE) == 0)
    {
        fprintf(stderr, "设置控制台事件处理失败\n");
        exit(1);
    }
#else
    // Linux/Unix: 使用信号处理
    if (signal(SIGINT, signal_handler) == SIG_ERR)
    {
        fprintf(stderr, "信号 SIGINT 设置失败\n");
        exit(1);
    }
    if (signal(SIGTERM, signal_handler) == SIG_ERR)
    {
        fprintf(stderr, "信号 SIGTERM 设置失败\n");
        exit(1);
    }
    if (signal(SIGHUP, signal_handler) == SIG_ERR)
    {
        fprintf(stderr, "信号 SIGHUP 设置失败\n");
        exit(1);
    }
    if (signal(SIGQUIT, signal_handler) == SIG_ERR)
    {
        fprintf(stderr, "信号 SIGQUIT 设置失败\n");
        exit(1);
    }
#endif
}