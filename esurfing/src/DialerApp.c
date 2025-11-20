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
            printf("参数错误: %c\n", optopt);
            return 1;
        default:
            printf("未知错误\n");
        }
    }
    loggerInit();
    if (username && password)
    {
        LOG_DEBUG("用户名: %s", usr);
        LOG_DEBUG("密码: %s", pwd);
        LOG_DEBUG("通道: %s", chn ? chn : "默认(pc)");
        if (smallDevice)
        {
            LOG_DEBUG("小容量设备模式已开启");
        }
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
                LOG_ERROR("通道参数错误");
                LOG_ERROR("请使用正确的参数运行程序");
                LOG_ERROR("格式: ESurfingClient -u <用户名> -p <密码> -c<通道>");
                shut(0);
                return 0;
            }
        }
        else
        {
            initChannel(1);
        }
        LOG_INFO("程序启动中");
        sleepMilliseconds(10000);
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
        LOG_ERROR("请使用正确的参数运行程序");
        LOG_ERROR("格式: ESurfingClient -u <用户名> -p <密码>");
    }
    shut(0);
}