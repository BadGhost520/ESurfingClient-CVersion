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
#include "headFiles/utils/ByteArray.h"
#include "headFiles/utils/XMLParser.h"

char* keepRetry;
char* keepUrl;
char* termUrl;
long long tick;

void authorization();

void term()
{
    NetResult* result = simplePost(termUrl, encrypt(createXMLPayload("term")));
    if (result->type == NET_RESULT_ERROR)
    {
        printf("登出错误\n");
    }
    free_net_result(result);
}

void heartbeat()
{
    NetResult* result = simplePost(keepUrl, encrypt(createXMLPayload("heartbeat")));
    if (result && result->type == NET_RESULT_SUCCESS)
    {
        free(keepRetry);
        keepRetry = XML_Parser(decrypt(result->data), "interval");
    }
    else
    {
        printf("[Client.c/heartbeat] result 为空");
    }
    free_net_result(result);
}

void login()
{
    NetResult* result = simplePost(authUrl, encrypt(createXMLPayload("login")));
    if (result && result->type == NET_RESULT_SUCCESS)
    {
        keepRetry = XML_Parser(decrypt(result->data), "keep-retry");
        keepUrl = cleanCDATA(XML_Parser(decrypt(result->data), "keep-url"));
        termUrl = cleanCDATA(XML_Parser(decrypt(result->data), "term-url"));
        printf("[Client.c/login] Keep Url: %s\n", keepUrl);
        printf("[Client.c/login] Term Url: %s\n", termUrl);
        printf("[Client.c/login] Keep Retry: %s\n", keepRetry);
    }
    else
    {
        printf("[Client.c/login] result 为空\n");
    }
    free_net_result(result);
}

char* getTicket()
{
    NetResult* result = simplePost(ticketUrl, encrypt(createXMLPayload("getTicket")));
    if (result && result->type == NET_RESULT_SUCCESS)
    {
        // printf("[Client.c/getTicket] data: %s\n", decrypt(result->data));
        free_net_result(result);
        return XML_Parser(decrypt(result->data), "ticket");
    }
    printf("[Client.c/getTicket] result 为空\n");
    free_net_result(result);
    return NULL;
}

void initSession()
{
    NetResult* result = simplePost(ticketUrl, algoId);
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
    if (keepUrl == NULL)
    {
        printf("[Client.c/authorization] Keep Url 为空\n");
        sessionFree();
        isRunning = false;
        return;
    }

    tick = currentTimeMillis();
    isLogged = true;
    printf("[Client.c/authorization] 已认证登录\n");
}

void run()
{
    while (isRunning)
    {
        const ConnectivityStatus networkStatus = detectConfig();
        switch (networkStatus)
        {
        case CONNECTIVITY_SUCCESS:
            if (isInitialized() && isLogged)
            {
                long long keep_retry;
                if (stringToLongLong(keepRetry, &keep_retry))
                {
                    if (currentTimeMillis() - tick >= keep_retry * 1000)
                    {
                        printf("[Client.c/run] 发送心跳包\n");
                        heartbeat();
                        printf("[Client.c/run] 下一次重试: %ss后\n", keepRetry);
                        tick = currentTimeMillis();
                    }
                }
                else
                {
                    printf("[Client.c/run] 字符串转int64错误");
                }
            }
            else
            {
                printf("[Client.c/run] 网络已连接\n");
            }
            sleepSeconds(1);
            break;
        case CONNECTIVITY_REQUIRE_AUTHORIZATION:
            printf("[Client.c/run] 需要认证\n");
            isLogged = false;
            authorization();
            break;
        case CONNECTIVITY_REQUEST_ERROR:
            printf("[Client.c/run] 网络错误\n");
            sleepSeconds(5);
            break;
        default:
            printf("[Client.c/run] 未知错误\n");
            sleepSeconds(5);
        }
    }
}