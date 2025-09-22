//
// Created by bad_g on 2025/9/14.
//
#include <stdio.h>

#include "headFiles/NetworkStatus.h"
#include "headFiles/PlatformUtils.h"
#include "headFiles/States.h"

void Auth(void);
void RefreshClientState(void);

void NetWorkStatus(void)
{
    ConnectivityStatus status;
    while (1)
    {
        status = detectConfig();
        switch (status)
        {
        case CONNECTIVITY_SUCCESS:
            printf("网络已连接\n");
            break;
        case CONNECTIVITY_REQUIRE_AUTHORIZATION:
            printf("需要认证\n");
            Auth();
            break;
        case CONNECTIVITY_REQUEST_ERROR:
            printf("网络错误\n");
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
    RefreshClientState();
    printf("ClientId: %s\n", clientId);
    printf("algoID: %s\n", algoId);
    printf("macAddress: %s\n", macAddress);
    printf("authUrl: %s\n", authUrl);
    printf("ticketUrl: %s\n", ticketUrl);
    printf("userIp: %s\n", userIp);
    printf("acIp: %s\n", acIp);
}

void InitSession()
{

}