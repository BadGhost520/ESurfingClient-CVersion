#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#include "headFiles/Client.h"

struct Options
{
    char* usr;
    char* pwd;
} Options;

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
            Options.usr = optarg;
            break;
        case 'p':
            password = true;
            Options.pwd = optarg;
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
        // printf("手机号：%s\n", Options.usr);
        // printf("密码：%s\n", Options.pwd);
        run();
    }
    else
    {
        printf("请使用正确的格式运行\n");
        printf("格式：ESurfingClient -u <手机号> -p <密码>");
    }
}