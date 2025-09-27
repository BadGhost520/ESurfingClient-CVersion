#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#include "headFiles/Client.h"
#include "headFiles/Constants.h"
#include "headFiles/utils/ShutdownHook.h"
#include "headFiles/Options.h"
#include "headFiles/Session.h"
#include "headFiles/States.h"

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
    while ((opt = getopt(argc, argv, "u:p:")) != -1)
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
        case '?':
            printf("参数错误：%c\n", optopt);
            return 1;
        default:
            printf("未知错误\n");
        }
    }

    if (username && password)
    {
        // printf("[DialerApp.c/main] 手机号：%s\n", Options.usr);
        // printf("[DialerApp.c/main] 密码：%s\n", Options.pwd);
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
        printf("请使用正确的格式运行\n");
        printf("格式：ESurfingClient -u <手机号> -p <密码>");
    }
    graceful_exit(0);
    return 0;
}