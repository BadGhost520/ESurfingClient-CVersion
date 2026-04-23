// #include "webserver/WebServer.h"
#include "utils/Shutdown.h"
#include "utils/Logger.h"
#include "States.h"

#include <signal.h>
#include <stdlib.h>

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
    default:
        LOG_DEBUG("接收到未处理的信号: %d", sig);
        shut(0);
    }
}

void shut(const uint8_t exitCode)
{
    LOG_INFO("主程序正在关闭");
    // if (is_webserver_running) stopWebServer();
    LOG_INFO("清理资源中");
    LOG_DEBUG("关闭线程保活");
    thread_keep_alive = false;
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

void init_shutdown_hook()
{
    LOG_DEBUG("初始化关闭钩子");
    if (signal(SIGINT, signal_handler) == SIG_ERR)
    {
        LOG_ERROR("信号 SIGINT");
        exit(1);
    }
    if (signal(SIGTERM, signal_handler) == SIG_ERR)
    {
        LOG_ERROR("信号 SIGTERM");
        exit(1);
    }
    LOG_DEBUG("初始化关闭钩子完成");
}
