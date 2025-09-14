//
// Created by bad_g on 2025/9/14.
//
#include <stdio.h>

#include "headFiles/NetworkStatus.h"
#include "headFiles/PlatformUtils.h"

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
            break;
        case 2:
            printf("连接失败\n");
            break;
        default:
            printf("未知错误\n");
        }
        sleep_seconds(10);
    }
}