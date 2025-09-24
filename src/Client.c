//
// Created by bad_g on 2025/9/14.
//
#include <stdio.h>
#include <string.h>

#include "headFiles/NetClient.h"
#include "headFiles/PlatformUtils.h"
#include "headFiles/Session.h"
#include "headFiles/States.h"
#include "headFiles/NetworkStatus.h"

void Auth(void);
void InitSession();

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
    InitSession();
}

void InitSession()
{
    NetResult* result = simple_post(ticketUrl, algoId);
    if (result && result->type == NET_RESULT_SUCCESS) {
        ByteArray zsm;
        zsm.data = (unsigned char*)result->data;
        zsm.length = strlen(result->data);
        SessionInitialize(&zsm);
        printf("ClientId: %s\n", clientId);
        printf("algoID: %s\n", algoId);
        printf("macAddress: %s\n", macAddress);
        printf("authUrl: %s\n", authUrl);
        printf("ticketUrl: %s\n", ticketUrl);
        printf("userIp: %s\n", userIp);
        printf("acIp: %s\n", acIp);
    } else {
        printf("请求失败: 无法创建结果对象\n");
    }

    if (result) {
        free_net_result(result);
    }
}