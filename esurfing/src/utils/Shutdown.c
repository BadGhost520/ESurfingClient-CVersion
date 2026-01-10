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
    if (thread_status[thread_local_args.thread_index].dialer_context.runtime_status.is_initialized)
    {
        if (thread_status[thread_local_args.thread_index].dialer_context.runtime_status.is_authed) term();
        freeSession();
    }
    LOG_DEBUG("清理资源中");
    memset(&thread_status[thread_local_args.thread_index].dialer_context, 0, sizeof(DialerContext));
    LOG_DEBUG("清理完成");
}

void checkAdapterStop()
{
    if (thread_status[thread_local_args.thread_index].need_stop)
    {
        LOG_INFO("认证程序 %d 正在关闭", thread_local_args.thread_index + 1);
        adapterStop();
        thread_status[thread_local_args.thread_index].need_stop = false;
        LOG_INFO("认证程序 %d 已关闭", thread_local_args.thread_index + 1);
    }
}

void shut(const int exitCode)
{
    if (thread_status[0].thread_is_running || thread_status[1].thread_is_running)
    {
        LOG_INFO("等待子线程关闭");
        for (int i = 0; i < MAX_DIALER_COUNT; i++)
        {
            thread_status[i].thread_is_running = false;
            thread_status[i].need_stop = true;
            waitThreadStop(i);
        }
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
