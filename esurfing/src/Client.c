//
// Created by bad_g on 2025/9/14.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "headFiles/States.h"
#include "headFiles/Session.h"
#include "headFiles/NetClient.h"
#include "headFiles/utils/Logger.h"
#include "headFiles/utils/PlatformUtils.h"
#include "headFiles/cipher/CipherInterface.h"

char* keepRetry;
char* keepUrl;
char* termUrl;
long long tick = 0;

void term()
{
    const char* encrypt = sessionEncrypt(createXMLPayload(Term));
    LOG_DEBUG("发送加密登出内容: %s", encrypt);
    HTTPResponse* result = simPost(termUrl, encrypt);
    if (result && result->status != RequestSuccess)
    {
        LOG_ERROR("登出错误，错误代码: %d", result->status);
    }
    isLogged = 0;
    freeResult(result);
}

void heartbeat()
{
    const char* encrypt = sessionEncrypt(createXMLPayload(Heartbeat));
    LOG_DEBUG("发送加密心跳内容: %s", encrypt);
    HTTPResponse* result = simPost(keepUrl, encrypt);
    if (result && result->status == RequestSuccess)
    {
        char* decrypted_data = sessionDecrypt(result->data);
        if (decrypted_data)
        {
            LOG_DEBUG("心跳响应内容: %s", decrypted_data);
            char* parsed_interval = XmlParser(decrypted_data, "interval");
            if (parsed_interval)
            {
                free(keepRetry);
                keepRetry = parsed_interval;
            }
            free(decrypted_data);
        }
        else
        {
            LOG_ERROR("解密心跳内容失败");
        }
    }
    else
    {
        LOG_ERROR("心跳响应失败，错误代码: %d", result->status);
    }
    freeResult(result);
}

void login()
{
    const char* encrypt = sessionEncrypt(createXMLPayload(Login));
    LOG_DEBUG("发送加密登录内容: %s", encrypt);
    HTTPResponse* result = simPost(authUrl, encrypt);
    if (result && result->status == RequestSuccess)
    {
        LOG_DEBUG("登录响应内容: %s", result->data);
        char* decrypted_data = sessionDecrypt(result->data);
        if (!decrypted_data)
        {
            LOG_ERROR("解密登录内容失败");
            freeResult(result);
            return;
        }
        
        char* parsed_keep_retry = XmlParser(decrypted_data, "keep-retry");
        if (parsed_keep_retry)
        {
            free(keepRetry);
            keepRetry = parsed_keep_retry;
        }
        
        char* parsed_keep_url = cleanCDATA(XmlParser(decrypted_data, "keep-url"));
        if (parsed_keep_url)
        {
            free(keepUrl);
            keepUrl = parsed_keep_url;
        }
        
        char* parsed_term_url = cleanCDATA(XmlParser(decrypted_data, "term-url"));
        if (parsed_term_url)
        {
            free(termUrl);
            termUrl = parsed_term_url;
        }
        
        LOG_INFO("Keep Url: %s", keepUrl ? keepUrl : "NULL");
        LOG_INFO("Term Url: %s", termUrl ? termUrl : "NULL");
        LOG_INFO("下一次重试: %s 秒后", keepRetry ? keepRetry : "NULL");
    }
    else
    {
        LOG_ERROR("登录响应失败，错误代码: %d", result->status);
    }
    freeResult(result);
}

void getTicket()
{
    const char* encrypt = sessionEncrypt(createXMLPayload(GetTicket));
    LOG_DEBUG("发送加密获取 ticket 内容: %s", encrypt);
    HTTPResponse* result = simPost(ticketUrl, encrypt);
    if (result && result->status == RequestSuccess)
    {
        LOG_DEBUG("获取 ticket 响应内容: %s", result->data);
        char* parsed_ticket = XmlParser(sessionDecrypt(result->data), "ticket");
        if (parsed_ticket)
        {
            ticket = strdup(parsed_ticket);
            free(parsed_ticket);
        }
        else
        {
            LOG_ERROR("获取 ticket 响应内容分析失败");
        }
    }
    else
    {
        LOG_ERROR("获取 ticket 响应失败，错误代码: %d", result->status);
    }
    freeResult(result);
}

void initSession()
{
    HTTPResponse* result = simPost(ticketUrl, algoId);
    if (result && result->status == RequestSuccess)
    {
        LOG_DEBUG("会话响应内容: %s", result->data);
        const ByteArray zsm = stringToBytes(result->data);
        initialize(&zsm);
        free(zsm.data);
    }
    else
    {
        LOG_ERROR("初始化会话失败，错误代码: %d", result->status);
    }
    freeResult(result);
}

void authorization()
{
    initSession();
    if (!isInitialized)
    {
        LOG_FATAL("会话初始化失败, 请重启程序, 重启你的设备或者重新从 Release 下载程序");
        LOG_FATAL("Release 网址: https://github.com/BadGhost520/ESurfingClient-CVersion/releases/latest");
        isRunning = 0;
        return;
    }
    LOG_INFO("Client IP: %s", userIp);
    LOG_INFO("AC IP: %s", acIp);
    getTicket();
    LOG_INFO("Ticket: %s", ticket);
    login();
    if (keepUrl == NULL)
    {
        LOG_FATAL("Keep Url 为空");
        sessionFree();
        isRunning = 0;
        return;
    }
    tick = currentTimeMillis();
    authTime = currentTimeMillis();
    LOG_DEBUG("登录时间戳 (毫秒): %lld", authTime);
    isLogged = 1;
    LOG_INFO("已认证登录");
}

void run()
{
    switch (checkNetworkStatus())
    {
    case RequestSuccess:
        if (isInitialized && isLogged)
        {
            long long keep_retry;
            if (stringToLongLong(keepRetry, &keep_retry))
            {
                if (currentTimeMillis() - tick >= keep_retry * 1000)
                {
                    LOG_INFO("发送心跳包");
                    heartbeat();
                    LOG_INFO("下一次重试: %s 秒后", keepRetry);
                    tick = currentTimeMillis();
                }
            }
            else
            {
                LOG_ERROR("String 转 int64 失败");
            }
        }
        else
        {
            LOG_INFO("网络已连接");
        }
        sleepMilliseconds(1000);
        break;
    case RequestAuthorization:
        LOG_INFO("需要认证");
        authorization();
        sleepMilliseconds(1000);
        break;
    case RequestError:
        LOG_ERROR("网络错误");
        sleepMilliseconds(5000);
        break;
    default:
        LOG_ERROR("未知错误");
        sleepMilliseconds(5000);
    }
}