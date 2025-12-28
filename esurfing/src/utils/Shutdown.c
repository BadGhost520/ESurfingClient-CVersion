#include <signal.h>
#include <stdlib.h>

#include "../headFiles/webserver/WebServer.h"
#include "../headFiles/utils/Shutdown.h"
#include "../headFiles/utils/Logger.h"
#include "../headFiles/DialerClient.h"
#include "../headFiles/Session.h"
#include "../headFiles/States.h"

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
    LOG_DEBUG("执行主线程关闭函数");
    if (is_webserver_running) stopWebServer();
    loggerCleanup();
}

static void adapterStop()
{
    LOG_DEBUG("执行关闭函数");
    dialer_adapter.runtime_status.is_running = 0;
    if (dialer_adapter.runtime_status.is_initialized)
    {
        if (dialer_adapter.runtime_status.is_logged) term();
        freeSession();
    }
}

void checkAdapterStop()
{
    if (adapter_need_stop[dialer_adapter.index])
    {
        adapter_need_stop[dialer_adapter.index] = false;
        adapterStop();
    }
}

void shut(const int exitCode)
{
    LOG_INFO("关闭主线程");
    mainStop();
    for (int i = 0; i < MAX_DIALER_COUNT; i++) adapter_need_stop[i] = true;
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
