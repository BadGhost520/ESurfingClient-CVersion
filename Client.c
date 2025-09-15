//
// Created by bad_g on 2025/9/14.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "headFiles/NetworkStatus.h"
#include "headFiles/PlatformUtils.h"

struct States
{
    char* clientId;
    char* algoId;
    char* macAddress;
} States;

void Auth(void);

void NetWorkStatus(void)
{
    int NetWorkStatus;
    while (1)
    {
        NetWorkStatus = checkNetworkStatus();
        switch (NetWorkStatus)
        {
        case 0:
            printf("连接成功\n");
            break;
        case 1:
            printf("需要认证\n");
            Auth();
            break;
        case 2:
            printf("连接失败\n");
            sleepSeconds(5);
            break;
        default:
            printf("未知错误\n");
            return;
        }
        sleepSeconds(10);
    }
}

void Auth(void)
{
    // 刷新客户端状态
    if (States.clientId)
    {
        free(States.clientId);
    }
    setClientId(&States.clientId);
    printf("%s\n", States.clientId);

    if (States.algoId)
    {
        free(States.algoId);
    }
    States.algoId = strdup("00000000-0000-0000-0000-000000000000");
    printf("%s\n", States.algoId);

    States.macAddress = strdup(randomMacAddress());
    printf("%s\n", States.macAddress);
}