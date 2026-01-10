#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "../headFiles/webserver/WebServer.h"
#include "../headFiles/utils/Shutdown.h"
#include "../headFiles/utils/Logger.h"
#include "../headFiles/DialerClient.h"
#include "../headFiles/NetClient.h"
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
    if (is_webserver_running) stopWebServer();
    loggerCleanup();
}

static void adapterStop()
{
    if (thread_status[thread_index].dialer_context.runtime_status.is_initialized)
    {
        if (thread_status[thread_index].dialer_context.runtime_status.is_authed) term();
        freeSession();
    }
    memset(&thread_status[thread_index].dialer_context, 0, sizeof(DialerContext));
    if (!thread_status[0].dialer_context.runtime_status.is_running && !thread_status[1].dialer_context.runtime_status.is_running)
    {
        memset(&last_location, 0, sizeof(last_location));
    }
}

void checkAdapterStop()
{
    if (thread_status[thread_index].need_stop)
    {
        LOG_INFO("线程 %d 正在关闭", thread_index + 1);
        thread_status[thread_index].need_stop = false;
        adapterStop();
    }
}

void shut(const int exitCode)
{
    if (thread_status[0].thread_is_running || thread_status[1].thread_is_running)
    {
        LOG_INFO("等待子线程关闭");
        for (int i = 0; i < MAX_DIALER_COUNT; i++) thread_status[i].need_stop = true;
        sleepMilliseconds(5000);
        checkThreadStatus();
    }
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
