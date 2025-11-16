#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "headFiles/Client.h"
#include "headFiles/Constants.h"
#include "headFiles/Options.h"
#include "headFiles/States.h"
#include "headFiles/utils/Logger.h"
#include "headFiles/utils/PlatformUtils.h"
#include "headFiles/utils/Shutdown.h"

int main(const int argc, char* argv[]) {
    int opt;
    int username = 0;
    int password = 0;
    int debugMode = 0;
    int channel = 0;
    while ((opt = getopt(argc, argv, "u:p:c::d")) != -1)
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
        isDebug = 1;
        loggerInit(LOG_LEVEL_DEBUG);
    }
    else
    {
        isDebug = 0;
        loggerInit(LOG_LEVEL_INFO);
    }
    if (username && password)
    {
        LOG_DEBUG("username: %s", usr);
        LOG_DEBUG("password: %s", pwd);
        LOG_DEBUG("Channel: %s", chn);
        if (channel)
        {
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
                shut(0);
                return 0;
            }
        }
        else
        {
            initChannel(1);
        }
        sleepMilliseconds(5000);
        isRunning = 1;
        initShutdown();
        initConstants();
        refreshStates();
        while (isRunning)
        {
            run();
        }
    }
    else
    {
        LOG_ERROR("Please run in the correct format");
        LOG_ERROR("Format: ESurfingClient -u <username> -p <password>");
    }
    shut(0);
}