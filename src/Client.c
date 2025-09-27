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
#include "headFiles/utils/Logger.h"
#include "headFiles/utils/ShutdownHook.h"

char* keepRetry;
char* keepUrl;
char* termUrl;
long long tick;
int retry = 0;

void authorization();

void term()
{
    NetResult* result = simplePost(termUrl, encrypt(createXMLPayload("term")));
    if (result->type == NET_RESULT_ERROR)
    {
        LOG_ERROR("登出错误");
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
        LOG_ERROR("result 为空");
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
        LOG_INFO("Keep Url: %s", keepUrl);
        LOG_INFO("Term Url: %s", termUrl);
        LOG_INFO("Keep Retry: %s", keepRetry);
    }
    else
    {
        LOG_ERROR("result 为空");
    }
    free_net_result(result);
}

char* getTicket()
{
    NetResult* result = simplePost(ticketUrl, encrypt(createXMLPayload("getTicket")));
    if (result && result->type == NET_RESULT_SUCCESS)
    {
        free_net_result(result);
        return XML_Parser(decrypt(result->data), "ticket");
    }
    LOG_ERROR("result 为空");
    free_net_result(result);
    return NULL;
}

void initSession()
{
    NetResult* result = simplePost(ticketUrl, algoId);
    if (result && result->type == NET_RESULT_SUCCESS)
    {
        const ByteArray zsm = string_to_bytes(result->data);
        if (retry == 5)
        {
            LOG_ERROR("程序多次匹配失败，请重新获取校园网IP地址，然后再打开本程序");
            graceful_exit(1);
        }
        if (initialize(&zsm) == 2 && retry < 5)
        {
            LOG_WARN("AlgoID匹配失败，正在重启程序");
            retry++;
            free_net_result(result);
            free(zsm.data);
            sleepSeconds(1);
            authorization();
        }
        retry = 0;
        free(zsm.data);
    } else {
        LOG_ERROR("初始化 Session 错误");
    }
    free_net_result(result);
}

void authorization()
{
    initSession();

    if (!isInitialized())
    {
        LOG_ERROR("初始化 Session 失败，请重启应用或从 Release 重新获取应用");
        LOG_ERROR("Release地址: https://github.com/BadGhost520/ESurfingClient-CVersion/releases");
        isRunning = false;
        return;
    }

    LOG_INFO("Client IP: %s", userIp);
    LOG_INFO("AC IP: %s", acIp);

    ticket = getTicket();
    LOG_INFO("Ticket: %s", ticket);

    login();
    if (keepUrl == NULL)
    {
        LOG_ERROR("Keep Url 为空");
        sessionFree();
        isRunning = false;
        return;
    }

    tick = currentTimeMillis();
    isLogged = true;
    LOG_INFO("已认证登录");
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
                        LOG_INFO("发送心跳包");
                        heartbeat();
                        LOG_INFO("下一次重试: %ss后", keepRetry);
                        tick = currentTimeMillis();
                    }
                }
                else
                {
                    LOG_ERROR("字符串转int64错误");
                }
            }
            else
            {
                LOG_INFO("网络已连接");
            }
            sleepSeconds(1);
            break;
        case CONNECTIVITY_REQUIRE_AUTHORIZATION:
            LOG_INFO("需要认证");
            isLogged = false;
            authorization();
            break;
        case CONNECTIVITY_REQUEST_ERROR:
            LOG_ERROR("网络错误");
            sleepSeconds(5);
            break;
        default:
            LOG_ERROR("未知错误");
            sleepSeconds(5);
        }
    }
}