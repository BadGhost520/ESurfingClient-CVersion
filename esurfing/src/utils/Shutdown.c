#include "../headFiles/utils/Shutdown.h"

#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

#include "../headFiles/cipher/CipherInterface.h"
#include "../headFiles/States.h"
#include "../headFiles/utils/Logger.h"
#include "../headFiles/Session.h"
#include "../headFiles/Client.h"

void performCleanup()
{
    LOG_DEBUG("执行关闭函数");
    if (isRunning)
    {
        isRunning = 0;
    }
    if (isInitialized)
    {
        if (isLogged)
        {
            term();
        }
        cipherFactoryDestroy();
        sessionFree();
    }
    loggerCleanup();
}

void shut(const int exitCode)
{
    LOG_INFO("正在关闭程序");
    performCleanup();
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
