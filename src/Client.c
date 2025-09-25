//
// Created by bad_g on 2025/9/14.
//
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "headFiles/NetClient.h"
#include "headFiles/PlatformUtils.h"
#include "headFiles/Session.h"
#include "headFiles/States.h"
#include "headFiles/NetworkStatus.h"
#include "headFiles/Constants.h"
#include "headFiles/utils/ByteArray.h"

void getTicket()
{
    char* payload = createXMLPayload();
    char* encry = encrypt(payload);
    printf("[Client.c/getTicket] 加密后数据:\n%s\n", encry);
    char* decry = decrypt(encry);
    printf("[Client.c/getTicket] 解密后数据:\n%s\n", decry);
    NetResult* result = simple_post(ticketUrl, encry);
    if (result && result->type == NET_RESULT_SUCCESS && result->data != NULL)
    {
        printf("[Client.c/getTicket] result.data: %s\n", result->data);
        char* data = decrypt(result->data);
        printf("[Client.c/getTicket] data: %s\n", data);
    }
    else
    {
        printf("[Client.c/getTicket] result 为空\n");
    }
    session_free();
    free(payload);
}

void initSession()
{
    NetResult* result = simple_post(ticketUrl, algoId);
    if (result && result->type == NET_RESULT_SUCCESS)
    {
        ByteArray temp = string_to_bytes(result->data);
        initialize(&temp);
        free(temp.data);
    } else {
        printf("[Client.c/initSession] 初始化 Session 错误");
    }
    free_net_result(result);
}

void authorization(void)
{
    initConstants();
    refreshStates();
    initSession();

    if (!isInitialized())
    {
        printf("[Client.c/authorization] 初始化 Session 失败，请重启应用或从 Release 重新获取应用\n");
        printf("[Client.c/authorization] Release地址: https://github.com/BadGhost520/ESurfingClient-CVersion/releases\n");
        isRunning = false;
        return;
    }

    printf("[Client.c/authorization] Client IP: %s\n", userIp);
    printf("[Client.c/authorization] AC IP: %s\n", acIp);

    getTicket();

    printf("[Client.c/authorization] ClientId: %s\n", clientId);
    printf("[Client.c/authorization] algoID: %s\n", algoId);
    printf("[Client.c/authorization] macAddress: %s\n", macAddress);
    printf("[Client.c/authorization] authUrl: %s\n", authUrl);
    printf("[Client.c/authorization] ticketUrl: %s\n", ticketUrl);


}

void run()
{
    while (1)
    {
        ConnectivityStatus networkStatus = detectConfig();
        switch (networkStatus)
        {
        case CONNECTIVITY_SUCCESS:
            printf("[Client.c/run] 网络已连接\n");
            break;
        case CONNECTIVITY_REQUIRE_AUTHORIZATION:
            printf("[Client.c/run] 需要认证\n");
            authorization();
            break;
        case CONNECTIVITY_REQUEST_ERROR:
            printf("[Client.c/run] 网络错误\n");
            sleepSeconds(5);
            break;
        default:
            printf("[Client.c/run] 未知错误\n");
            return;
        }
        sleepSeconds(10);
    }
}