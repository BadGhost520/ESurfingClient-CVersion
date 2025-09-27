#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#include "headFiles/Client.h"
#include "headFiles/Constants.h"
#include "headFiles/utils/ShutdownHook.h"
#include "headFiles/Options.h"
#include "headFiles/Session.h"
#include "headFiles/States.h"
#include "headFiles/cipher/cipher_interface.h"
#include "headFiles/utils/Logger.h"

void shutdownHook()
{
    if (isRunning)
    {
        isRunning = false;
    }
    if (isInitialized)
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
    bool username = false;
    bool password = false;
    bool debugMode = false;
    while ((opt = getopt(argc, argv, "u:p:d")) != -1)
    {
        switch (opt)
        {
        case 'u':
            username = true;
            usr = optarg;
            break;
        case 'p':
            password = true;
            pwd = optarg;
            break;
        case 'd':
            debugMode = true;
            printf("Debug模式已开启\n");
            break;
        case '?':
            printf("参数错误：%c\n", optopt);
            return 1;
        default:
            printf("未知错误\n");
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

    if (username && password)
    {
        LOG_DEBUG("手机号：%s", usr);
        LOG_DEBUG("密码：%s", pwd);
        initShutdownHook();
        while (isRunning) {
            // 你的业务逻辑
            initConstants();
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
        LOG_INFO("请使用正确的格式运行");
        LOG_INFO("格式：ESurfingClient -u <手机号> -p <密码>");
    }
    graceful_exit(0);
    return 0;
}