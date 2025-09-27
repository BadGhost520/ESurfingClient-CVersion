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
#include "headFiles/utils/XMLParser.h"

void authorization();

void login()
{
    char* payload = createXMLPayload("login");
    NetResult* result = simple_post(authUrl, encrypt(payload));
    if (result && result->type == NET_RESULT_SUCCESS && result->data != NULL)
    {
        char* keepRetry = XML_Parser(decrypt(result->data), "keep-retry");
        char* keepUrl = cleanCDATA(XML_Parser(decrypt(result->data), "keep-url"));
        char* termUrl = cleanCDATA(XML_Parser(decrypt(result->data), "term-url"));
        printf("Keep Url: %s\n", keepUrl);
        printf("Term Url: %s\n", termUrl);
        printf("Keep Retry: %s\n", keepRetry);
    }
    else
    {
        printf("[Client.c/login] result 为空\n");
    }
    free_net_result(result);
    free(payload);
}

char* getTicket()
{
    char* payload = createXMLPayload("getTicket");
    NetResult* result = simple_post(ticketUrl, encrypt(payload));
    if (result && result->type == NET_RESULT_SUCCESS && result->data != NULL)
    {
        // printf("[Client.c/getTicket] data: %s\n", decrypt(result->data));
        free_net_result(result);
        return XML_Parser(decrypt(result->data), "ticket");
    }
    printf("[Client.c/getTicket] result 为空\n");
    free_net_result(result);
    free(payload);
    return NULL;
}

void initSession()
{
    NetResult* result = simple_post(ticketUrl, algoId);
    if (result && result->type == NET_RESULT_SUCCESS)
    {
        const ByteArray zsm = string_to_bytes(result->data);
        // printf("[Client.c/initSession] result.data: %s\n", result->data);
        if (initialize(&zsm) == 2)
        {
            printf("[Client.c/initSession] AlgoID匹配失败，正在重新执行程序\n");
            free_net_result(result);
            free(zsm.data);
            authorization();
        }
        free(zsm.data);
    } else {
        printf("[Client.c/initSession] 初始化 Session 错误\n");
    }
    free_net_result(result);
}

void authorization()
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

    ticket = getTicket();
    printf("[Client.c/authorization] Ticket: %s\n", ticket);

    login();

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