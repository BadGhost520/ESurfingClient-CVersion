#include <locale.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "headFiles/Client.h"
#include "headFiles/Constants.h"
#include "headFiles/Options.h"
#include "headFiles/Session.h"
#include "headFiles/States.h"
#include "headFiles/cipher/CipherInterface.h"
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
            isSmallDevice = 1;
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
        LOG_DEBUG("通道: %s", chn ? chn : "默认 (phone)");
        if (isSmallDevice)
        {
            LOG_DEBUG("小容量设备模式已开启");
        }
        if (channel)
        {
            if (strcmp(chn, "pc") == 0)
            {
                USER_AGENT = "CCTP/Linux64/1003";
            }
            else if (strcmp(chn, "phone") == 0)
            {
                USER_AGENT = "CCTP/android64_vpn/2093";
            }
            else
            {
                LOG_FATAL("通道参数错误");
                LOG_WARN("请使用正确的参数运行程序");
                LOG_WARN("格式: ESurfingClient -u <用户名> -p <密码> -c<通道>");
                LOG_WARN("或不使用 -c 参数，将使用默认通道 (phone)");
                shut(0);
                return 0;
            }
        }
        else
        {
            USER_AGENT = "CCTP/android64_vpn/2093";
        }
        LOG_DEBUG("UA: %s", USER_AGENT);
        if (access("/etc/openwrt_release", F_OK) == 0)
        {
            createBash();
        }
        isRunning = 1;
        initShutdown();
        initConstants();
        refreshStates();
        LOG_INFO("程序启动中");
        sleepMilliseconds(5000);
        while (isRunning)
        {
            // 172200000
            if (currentTimeMillis() - authTime >= 120000 && authTime != 0)
            {
                LOG_DEBUG("当前时间戳: %lld", currentTimeMillis());
                LOG_WARN("已登录 2870 分钟(1 天 23 小时 50 分钟)，为避免被远程服务器踢下线，正在重新进行认证");
                if (isInitialized)
                {
                    if (isLogged)
                    {
                        term();
                    }
                    cipherFactoryDestroy();
                    sessionFree();
                }
                authTime = 0;
                sleepMilliseconds(5000);
                initConstants();
                refreshStates();
            }
            run();
        }
    }
    else
    {
        LOG_FATAL("参数错误");
        LOG_WARN("请使用正确的参数运行程序");
        LOG_WARN("格式: ESurfingClient -u <用户名> -p <密码>");
    }
    shut(0);
}