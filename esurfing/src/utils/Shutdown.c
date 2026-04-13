#include "webserver/WebServer.h"
#include "utils/Shutdown.h"
#include "utils/Logger.h"
#include "States.h"

#include <signal.h>
#include <stdlib.h>

static void signalHandler(const int sig)
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

static void mainStop()
{
    if (is_webserver_running) stopWebServer();
    loggerCleanup();
}

void shut(const int exitCode)
{
    LOG_INFO("主线程正在关闭");
    mainStop();
    exit(exitCode);
}

void initShutdown()
{
    if (signal(SIGINT, signalHandler) == SIG_ERR)
    {
        LOG_ERROR("信号 SIGINT");
        exit(1);
    }
    if (signal(SIGTERM, signalHandler) == SIG_ERR)
    {
        LOG_ERROR("信号 SIGTERM");
        exit(1);
    }
}
