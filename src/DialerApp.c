#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#include "headFiles/Client.h"
#include "headFiles/utils/ShutdownHook.h"
#include "headFiles/Options.h"

void shutdownHook()
{
    exit(0);
}

void setShutdownHook()
{
    user_cleanup_function = shutdownHook;
    addShutdownHook();
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
        setShutdownHook();
        run();
    }
    else
    {
        printf("请使用正确的格式运行\n");
        printf("格式：ESurfingClient -u <手机号> -p <密码>");
    }
}