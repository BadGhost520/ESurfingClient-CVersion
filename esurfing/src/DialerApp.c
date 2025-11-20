#include <locale.h>
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
    int channel = 0;

#ifdef _WIN32
    system("chcp 65001 > nul");
#endif

    printf("中文测试\n");

    while ((opt = getopt(argc, argv, "u:p:c::ds")) != -1)
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
            isDebug = 1;
            break;
        case 's':
            smallDevice = 1;
            break;
        case '?':
            printf("Parameter error：%c\n", optopt);
            return 1;
        default:
            printf("Unknown error\n");
        }
    }
    loggerInit();
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
        LOG_INFO("Progress starting");
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