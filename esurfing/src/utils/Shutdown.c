#include "../headFiles/utils/Shutdown.h"

#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

#include "../headFiles/States.h"
#include "../headFiles/utils/Logger.h"
#include "../headFiles/Session.h"
#include "../headFiles/Client.h"

void performCleanup()
{
    LOG_DEBUG("Executing the close function");
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
        sessionFree();
    }
    loggerCleanup();
}

void shut(const int exitCode)
{
    LOG_INFO("Shutting down");
    performCleanup();
    exit(exitCode);
}

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
