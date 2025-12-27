#include <signal.h>
#include <stdlib.h>

#include "../headFiles/cipher/CipherInterface.h"
#include "../headFiles/webserver/WebServer.h"
#include "../headFiles/utils/Shutdown.h"
#include "../headFiles/utils/Logger.h"
#include "../headFiles/Session.h"
#include "../headFiles/States.h"
#include "../headFiles/Client.h"

void adapterStop()
{
    for (int i = 0; i < MAX_DIALERS; i++)
    {
        LOG_DEBUG("执行关闭函数");
        if (g_dialer_adapter[i].runtime_status.is_running) g_dialer_adapter[i].runtime_status.is_running = 0;
        if (g_dialer_adapter[i].runtime_status.is_initialized)
        {
            if (g_dialer_adapter[i].runtime_status.is_logged) term();
            cipherFactoryDestroy();
            freeSession();
        }
    }
}

void mainStop()
{
    if (is_webserver_running)
    {
        stopWebServer();
    }
    loggerCleanup();
}

void shut(const int exitCode)
{

    LOG_INFO("关闭主线程");
    mainStop();
    exit(exitCode);
}

void signalHandler(const int sig)
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
