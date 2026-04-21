// #include "webserver/WebServer.h"
#include "utils/Shutdown.h"
#include "utils/Logger.h"
#include "DialerClient.h"
#include "States.h"

#include <inttypes.h>
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
    g_shut_lock = true;
    LOG_INFO("清理资源中");
    LOG_DEBUG("开始配置清理");
    for (g_prog_idx = 0; g_prog_idx < g_prog_cnt; g_prog_idx++)
    {
        LOG_DEBUG("操作配置 %" PRIu8, g_prog_status[g_prog_idx].login_cfg.idx);
        if (g_prog_status[g_prog_idx].runtime_status.is_authed)
        {
            LOG_DEBUG("配置 %" PRIu8 " 登出", g_prog_status[g_prog_idx].login_cfg.idx);
            term();
        }
    }
    g_prog_idx = 0;
    LOG_DEBUG("操作完成");
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
