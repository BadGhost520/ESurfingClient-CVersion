#include <stdlib.h>
#include <string.h>

#include "headFiles/cipher/CipherInterface.h"
#include "headFiles/utils/PlatformUtils.h"
#include "headFiles/utils/Logger.h"
#include "headFiles/NetClient.h"
#include "headFiles/Session.h"
#include "headFiles/States.h"
#include "headFiles/Client.h"

static ClientData client_data = {0};

void term(DialerContext* adapter)
{
    char* payload;
    createXMLPayload(Term, &payload);
    if (payload)
    {
        char* encrypt;
        sessionEncrypt(payload, &encrypt);
        if (encrypt)
        {
            LOG_DEBUG("发送加密登出内容: %s", encrypt);
            HTTPResponse* result = simPost(client_data.term_url, encrypt);
            if (result && result->status != RequestSuccess) LOG_ERROR("登出错误，错误代码: %d", result->status);
            adapter->runtime_status.is_logged = 0;
            freeResult(result);
            free(encrypt);
        }
        free(payload);
    }
}

static void heartbeat()
{
    char* payload;
    createXMLPayload(Heartbeat, &payload);
    if (payload)
    {
        char* encrypt;
        sessionEncrypt(payload, &encrypt);
        if (encrypt)
        {
            LOG_DEBUG("发送加密心跳内容: %s", encrypt);
            HTTPResponse* result = simPost(client_data.keep_url, encrypt);
            if (result && result->status == RequestSuccess)
            {
                char* decrypted_data;
                sessionDecrypt(result->data, &decrypted_data);
                if (decrypted_data)
                {
                    LOG_DEBUG("心跳响应内容: %s", decrypted_data);
                    char* parsed_interval;
                    XmlParser(decrypted_data, "interval", &parsed_interval);
                    if (parsed_interval)
                    {
                        free(client_data.keep_retry);
                        client_data.keep_retry = strdup(parsed_interval);
                        free(parsed_interval);
                    }
                    free(decrypted_data);
                }
                else
                    LOG_ERROR("解密心跳内容失败");
            }
            else
                LOG_ERROR("心跳响应失败，错误代码: %d", result->status);
            freeResult(result);
            free(encrypt);
        }
        free(payload);
    }
}

static void login()
{
    char* payload;
    createXMLPayload(Login, &payload);
    if (payload)
    {
        char* encrypt;
        sessionEncrypt(payload, &encrypt);
        if (encrypt)
        {
            LOG_DEBUG("发送加密登录内容: %s", encrypt);
            HTTPResponse* result = simPost(dialer_adapter.auth_config.auth_url, encrypt);
            if (result && result->status == RequestSuccess)
            {
                LOG_DEBUG("登录响应内容: %s", result->data);
                char* decrypted_data;
                sessionDecrypt(result->data, &decrypted_data);
                if (decrypted_data)
                {
                    char* parsed_keep_retry;
                    XmlParser(decrypted_data, "keep-retry", &parsed_keep_retry);
                    if (parsed_keep_retry)
                    {
                        free(client_data.keep_retry);
                        client_data.keep_retry = strdup(parsed_keep_retry);
                        free(parsed_keep_retry);
                    }
                    char* parsed_keep_url;
                    XmlParser(decrypted_data, "keep-url", &parsed_keep_url);
                    if (parsed_keep_url)
                    {
                        char* cleaned_keep_url;
                        cleanCDATA(parsed_keep_url, &cleaned_keep_url);
                        if (cleaned_keep_url)
                        {
                            free(client_data.keep_url);
                            client_data.keep_url = strdup(cleaned_keep_url);
                            free(cleaned_keep_url);
                        }
                        free(parsed_keep_url);
                    }
                    char* parsed_term_url;
                    XmlParser(decrypted_data, "term-url", &parsed_term_url);
                    if (parsed_term_url)
                    {
                        char* cleaned_term_url;
                        cleanCDATA(parsed_term_url, &cleaned_term_url);
                        if (cleaned_term_url)
                        {
                            free(client_data.term_url);
                            client_data.term_url = strdup(cleaned_term_url);
                            free(cleaned_term_url);
                        }
                        free(parsed_term_url);
                    }
                    LOG_INFO("Keep Url: %s", client_data.keep_url ? client_data.keep_url : "NULL");
                    LOG_INFO("Term Url: %s", client_data.term_url ? client_data.term_url : "NULL");
                    LOG_INFO("下一次重试: %s 秒后", client_data.keep_retry ? client_data.keep_retry : "NULL");
                    free(decrypted_data);
                }
            }
            else
                LOG_ERROR("登录响应失败，错误代码: %d", result->status);
            freeResult(result);
            free(encrypt);
        }
        free(payload);
    }
}

static void getTicket()
{
    char* payload;
    createXMLPayload(GetTicket, &payload);
    if (payload)
    {
        char* encrypt;
        sessionEncrypt(payload, &encrypt);
        if (encrypt)
        {
            LOG_DEBUG("发送加密获取 ticket 内容: %s", encrypt);
            HTTPResponse* result = simPost(dialer_adapter.auth_config.auth_url, encrypt);
            if (result && result->status == RequestSuccess)
            {
                LOG_DEBUG("获取 ticket 响应内容: %s", result->data);
                char* decrypt;
                sessionDecrypt(result->data, &decrypt);
                if (decrypt)
                {
                    char* parsed_ticket;
                    XmlParser(decrypt, "ticket", &parsed_ticket);
                    if (parsed_ticket)
                    {
                        dialer_adapter.auth_config.ticket = strdup(parsed_ticket);
                        free(parsed_ticket);
                    }
                    else
                        LOG_ERROR("获取 ticket 响应内容分析失败");
                    free(decrypt);
                }
            }
            else
                LOG_ERROR("获取 ticket 响应失败，错误代码: %d", result->status);
            freeResult(result);
            free(encrypt);
        }
        free(payload);
    }
}

static void initSession()
{
    HTTPResponse* result = simPost(dialer_adapter.auth_config.ticket_url, dialer_adapter.auth_config.algo_id);
    if (result && result->status == RequestSuccess)
    {
        LOG_DEBUG("会话响应内容: %s", result->data);
        const ByteArray zsm = stringToBytes(result->data);
        initialize(&zsm);
        free(zsm.data);
    }
    else
        LOG_ERROR("初始化会话失败，错误代码: %d", result->status);
    freeResult(result);
}

static void authorization()
{
    initSession();
    if (!dialer_adapter.runtime_status.is_initialized)
    {
        LOG_FATAL("会话初始化失败, 请重启程序, 重启你的设备或者重新从 Release 下载程序");
        LOG_FATAL("Release 网址: https://github.com/BadGhost520/ESurfingClient-CVersion/releases/latest");
        dialer_adapter.runtime_status.is_running = false;
        return;
    }
    LOG_INFO("Client IP: %s", dialer_adapter.auth_config.user_ip);
    LOG_INFO("AC IP: %s", dialer_adapter.auth_config.ac_ip);
    getTicket();
    LOG_INFO("Ticket: %s", dialer_adapter.auth_config.ticket);
    login();
    if (client_data.keep_url == NULL)
    {
        LOG_FATAL("Keep Url 为空");
        sessionFree();
        dialer_adapter.runtime_status.is_running = false;
        return;
    }
    client_data.tick = currentTimeMillis();
    dialer_adapter.timestamp.auth_time = currentTimeMillis();
    LOG_DEBUG("登录时间戳 (毫秒): %lld", dialer_adapter.timestamp.auth_time);
    dialer_adapter.runtime_status.is_logged = true;
    LOG_INFO("已认证登录");
}

void run()
{
    switch (checkNetworkStatus())
    {
    case RequestSuccess:
        if (dialer_adapter.runtime_status.is_initialized && dialer_adapter.runtime_status.is_logged)
        {
            long long keep_retry;
            if (stringToLongLong(client_data.keep_retry, &keep_retry))
            {
                if (currentTimeMillis() - client_data.tick >= keep_retry * 1000)
                {
                    LOG_INFO("发送心跳包");
                    heartbeat();
                    LOG_INFO("下一次重试: %s 秒后", client_data.keep_retry);
                    client_data.tick = currentTimeMillis();
                }
            }
            else
                LOG_ERROR("String 转 int64 失败");
        }
        else
            LOG_INFO("网络已连接");
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
    case InitError:
        LOG_ERROR("初始化错误");
        sleepMilliseconds(5000);
        break;
    default:
        LOG_ERROR("未知错误");
        sleepMilliseconds(5000);
    }
}