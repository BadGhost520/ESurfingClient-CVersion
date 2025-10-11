#include <stdio.h>
#include <unistd.h>

#include "headFiles/Client.h"
#include "headFiles/Constants.h"
#include "headFiles/utils/ShutdownHook.h"
#include "headFiles/Options.h"
#include "headFiles/PlatformUtils.h"
#include "headFiles/Session.h"
#include "headFiles/States.h"
#include "headFiles/utils/Logger.h"

void shutdownHook()
{
    sleepSeconds(1);
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
    logger_cleanup();
}

void initShutdownHook()
{
    init_shutdown_hook();
    register_cleanup_function(shutdownHook);
}

int main(const int argc, char* argv[]) {
    int opt;
    int username = 0;
    int password = 0;
    int debugMode = 0;
    int channel = 0;
    while ((opt = getopt(argc, argv, "u:p:c:d")) != -1)
    {
        switch (opt)
        {
        case 'u':
            username = 1;
            usr = optarg;
            break;
        case 'p':
            password = 1;
            pwd = optarg;
            break;
        case 'c':
            channel = 1;
            chn = optarg;
            break;
        case 'd':
            debugMode = 1;
            printf("Debug mode is enabled\n");
            break;
        case '?':
            printf("Parameter error：%c\n", optopt);
            return 1;
        default:
            printf("Unknown error\n");
        }
    }

    if (debugMode)
    {
        logger_init(LOG_LEVEL_DEBUG, LOG_TARGET_BOTH, "run.log");
    }
    else
    {
        logger_init(LOG_LEVEL_INFO, LOG_TARGET_BOTH, "run.log");
    }

    initShutdownHook();

    if (username && password && channel)
    {
        LOG_DEBUG("username: %s", usr);
        LOG_DEBUG("password: %s", pwd);
        if (strcmp(chn, "pc") == 0)
        {
            initConstants(1);
            LOG_DEBUG("UA: %s", USER_AGENT);
        }
        else if (strcmp(chn, "phone") == 0)
        {
            initConstants(2);
            LOG_DEBUG("UA: %s", USER_AGENT);
        }
        else
        {
            LOG_ERROR("Wrong channel");
            LOG_ERROR("Please run in the correct format");
            LOG_ERROR("Format: ESurfingClient -u <username> -p <password> -c <channel>");
            LOG_ERROR("<channel>: 1.pc, 2.phone");
            graceful_exit(0);
            return 0;
        }
        while (isRunning) {
            // 你的业务逻辑

            refreshStates();
            run();
            // 重要：检查退出条件
            if (!isRunning) {
                break;
            }
        }
    }
    else
    {
        LOG_ERROR("Please run in the correct format");
        LOG_ERROR("Format: ESurfingClient -u <username> -p <password> -c <channel>");
        LOG_ERROR("<channel>: 1.pc, 2.phone");
    }
    graceful_exit(0);
    return 0;
}