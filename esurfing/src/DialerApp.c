#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "headFiles/Client.h"
#include "headFiles/Constants.h"
#include "headFiles/Options.h"
#include "headFiles/utils/PlatformUtils.h"
#include "headFiles/States.h"
#include "headFiles/utils/Logger.h"
#include "headFiles/utils/Shutdown.h"

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
        logger_init(LOG_LEVEL_DEBUG, LOG_TARGET_BOTH);
    }
    else
    {
        logger_init(LOG_LEVEL_INFO, LOG_TARGET_BOTH);
    }

    initShutdown();

    if (username && password && channel)
    {
        LOG_DEBUG("username: %s", usr);
        LOG_DEBUG("password: %s", pwd);
        if (strcmp(chn, "pc") == 0)
        {
            initChannel(1);
        }
        else if (strcmp(chn, "phone") == 0)
        {
            initChannel(2);
        }
        else
        {
            LOG_ERROR("Wrong channel");
            LOG_ERROR("Please run in the correct format");
            LOG_ERROR("Format: ESurfingClient -u <username> -p <password> -c <channel>");
            LOG_ERROR("<channel>: 1.pc, 2.phone");
            shut(0);
            return 0;
        }
        while (isRunning) {
            initConstants();
            refreshStates();
            run();
        }
    }
    else
    {
        LOG_ERROR("Please run in the correct format");
        LOG_ERROR("Format: ESurfingClient -u <username> -p <password> -c <channel>");
        LOG_ERROR("<channel>: 1.pc, 2.phone");
    }
    shut(0);
}