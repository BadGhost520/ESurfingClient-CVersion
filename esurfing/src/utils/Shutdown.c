#include "../headFiles/utils/Shutdown.h"

#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

#include "../headFiles/States.h"
#include "../headFiles/utils/Logger.h"
#include "../headFiles/utils/PlatformUtils.h"
#include "../headFiles/Session.h"
#include "../headFiles/Client.h"

/**
 * 关闭时要执行的函数
 */
void performCleanup()
{
    LOG_DEBUG("Executing the close function");
    if (isRunning)
    {
        isRunning = 0;
    }
    if (isInitialized())
    {
        if (isLogged)
        {
            term();
        }
        sessionFree();
    }
    loggerCleanup();
}

/**
 * 关闭函数
 * @param exitCode 退出码
 */
void shut(const int exitCode)
{
    LOG_INFO("Shutting down");
    performCleanup();
    exit(exitCode);
}

/**
 * 信号处理函数
 * @param sig 信号
 */
void signalHandler(const int sig)
{
    switch(sig)
    {
        case SIGINT:
            LOG_DEBUG("Received SIGINT signal (Ctrl+C)");
            shut(0);
        case SIGTERM:
            LOG_DEBUG("Received SIGTERM signal (Terminate request)");
            shut(0);
        default:
            LOG_DEBUG("Received unprocessed signal: %d", sig);
            shut(0);
    }
}

/**
 * 初始化关闭函数
 */
void initShutdown()
{
    if (signal(SIGINT, signalHandler) == SIG_ERR)
    {
        LOG_ERROR("signal SIGINT");
        exit(1);
    }

    if (signal(SIGTERM, signalHandler) == SIG_ERR)
    {
        LOG_ERROR("signal SIGTERM");
        exit(1);
    }
}
