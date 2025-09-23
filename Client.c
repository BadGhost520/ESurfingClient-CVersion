//
// Created by bad_g on 2025/9/14.
//
#include <stdio.h>
#include <string.h>

#include "headFiles/NetworkStatus.h"
#include "headFiles/PlatformUtils.h"
#include "headFiles/States.h"
#include "headFiles/NetClient.h"

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
    printf("ClientId: %s\n", clientId);
    printf("algoID: %s\n", algoId);
    printf("macAddress: %s\n", macAddress);
    printf("authUrl: %s\n", authUrl);
    printf("ticketUrl: %s\n", ticketUrl);
    printf("userIp: %s\n", userIp);
    printf("acIp: %s\n", acIp);
    NetResult* result1 = simple_post(ticketUrl, algoId);

    if (result1 && result1->type == NET_RESULT_SUCCESS) {
        printf("请求成功!\n");
        printf("状态码: %d\n", result1->status_code);
        if (result1->data) {
            printf("响应长度: %zu 字符\n", strlen(result1->data));
        } else {
            printf("响应数据为空\n");
        }
    } else if (result1) {
        printf("请求失败: %s\n", result1->error_message ? result1->error_message : "未知错误");
    } else {
        printf("请求失败: 无法创建结果对象\n");
    }

    if (result1) {
        free_net_result(result1);
    }
}