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

RunningStatus term()
{
    char* payload = createXMLPayload(TERM);
    if (!payload)
    {
        LOG_ERROR("登出 XML 创建失败");
        return RUNNING_FAILURE;
    }
    char* encrypt = sessionEncrypt(payload);
    free(payload);
    if (!encrypt)
    {
        LOG_ERROR("登出 XML 加密失败");
        return RUNNING_FAILURE;
    }
    LOG_DEBUG("发送加密登出内容: %s", encrypt);
    HTTPResponse* result = simPost(client_data.term_url, encrypt);
    free(encrypt);
    if (!result || result->status == REQUEST_ERROR)
    {
        LOG_ERROR("登出错误，错误代码: %d", result->status);
        freeResult(result);
        return RUNNING_FAILURE;
    }
    freeResult(result);
    dialer_adapter.runtime_status.is_logged = 0;
    return RUNNING_SUCCESS;
}

static RunningStatus heartbeat()
{
    char* payload = createXMLPayload(HEART_BEAT);
    if (!payload)
    {
        LOG_ERROR("心跳 XML 创建失败");
        return RUNNING_FAILURE;
    }
    char* encrypt = sessionEncrypt(payload);
    free(payload);
    if (!encrypt)
    {
        LOG_ERROR("加密心跳 XML 失败");
        return RUNNING_FAILURE;
    }
    LOG_DEBUG("发送加密心跳内容: %s", encrypt);
    HTTPResponse* result = simPost(client_data.keep_url, encrypt);
    free(encrypt);
    if (!result || result->status == REQUEST_ERROR)
    {
        LOG_ERROR("心跳响应失败，错误代码: %d", result->status);
        freeResult(result);
        return RUNNING_FAILURE;
    }
    char* decrypted_data = sessionDecrypt(result->data);
    freeResult(result);
    if (!decrypted_data)
    {
        LOG_ERROR("解密心跳内容失败");
        return RUNNING_FAILURE;
    }
    LOG_DEBUG("心跳响应内容: %s", decrypted_data);
    char* parsed_interval = XmlParser(decrypted_data, "interval");
    free(decrypted_data);
    if (!parsed_interval)
    {
        LOG_ERROR("心跳内容解析失败");
        return RUNNING_FAILURE;
    }
    char* new_interval = strdup(parsed_interval);
    free(parsed_interval);
    if (!new_interval)
    {
        LOG_ERROR("复制心跳间隔失败");
        return RUNNING_FAILURE;
    }
    if (client_data.keep_retry) free(client_data.keep_retry);
    client_data.keep_retry = new_interval;
    return RUNNING_SUCCESS;
}

static AuthStatus login()
{
    char* payload = createXMLPayload(LOGIN);
    if (!payload)
    {
        LOG_ERROR("登录 XML 创建失败");
        return AUTH_FAILURE;
    }
    char* encrypt = sessionEncrypt(payload);
    free(payload);
    if (!encrypt)
    {
        LOG_ERROR("加密登录 XML 失败");
        return AUTH_FAILURE;
    }
    LOG_DEBUG("发送加密登录内容: %s", encrypt);
    HTTPResponse* result = simPost(dialer_adapter.auth_config.auth_url, encrypt);
    free(encrypt);
    if (!result || result->status == REQUEST_ERROR)
    {
        LOG_ERROR("登录响应失败，错误代码: %d", result->status);
        freeResult(result);
        return AUTH_FAILURE;
    }
    LOG_DEBUG("登录响应内容: %s", result->data);
    char* decrypted_data = sessionDecrypt(result->data);
    freeResult(result);
    if (!decrypted_data)
    {
        LOG_ERROR("解密登录响应内容失败");
        return AUTH_FAILURE;
    }
    char* parsed_keep_retry = XmlParser(decrypted_data, "keep-retry");
    if (!parsed_keep_retry)
    {
        LOG_ERROR("解析 KeepRetry 失败");
        return AUTH_FAILURE;
    }
    char* new_keep_retry = strdup(parsed_keep_retry);
    free(parsed_keep_retry);
    if (!new_keep_retry)
    {
        LOG_ERROR("复制新 KeepRetry 失败");
        return AUTH_FAILURE;
    }
    if (client_data.keep_retry) free(client_data.keep_retry);
    client_data.keep_retry = new_keep_retry;
    char* parsed_keep_url = XmlParser(decrypted_data, "keep-url");
    if (!parsed_keep_url)
    {
        LOG_ERROR("解析 KeepURL 失败");
        return AUTH_FAILURE;
    }
    char* cleaned_keep_url = cleanCDATA(parsed_keep_url);
    free(parsed_keep_url);
    if (!cleaned_keep_url)
    {
        LOG_ERROR("清除 KeepURL CDATA 失败");
        return AUTH_FAILURE;
    }
    char* new_keep_url = strdup(cleaned_keep_url);
    free(cleaned_keep_url);
    if (!new_keep_url)
    {
        LOG_ERROR("复制新 KeepURL 失败");
        return AUTH_FAILURE;
    }
    if (client_data.keep_url) free(client_data.keep_url);
    client_data.keep_url = new_keep_url;
    char* parsed_term_url = XmlParser(decrypted_data, "term-url");
    free(decrypted_data);
    if (!parsed_term_url)
    {
        LOG_ERROR("解析 TermURL 失败");
        return AUTH_FAILURE;
    }
    char* cleaned_term_url = cleanCDATA(parsed_term_url);
    free(parsed_term_url);
    if (!cleaned_term_url)
    {
        LOG_ERROR("清除 TermURL CDATA 失败");
        return AUTH_FAILURE;
    }
    char* new_term_url = strdup(cleaned_term_url);
    free(cleaned_term_url);
    if (!new_term_url)
    {
        LOG_ERROR("复制新 TermURL 失败");
        return AUTH_FAILURE;
    }
    if (client_data.term_url) free(client_data.term_url);
    client_data.term_url = new_term_url;
    LOG_INFO("Keep Url: %s", client_data.keep_url ? client_data.keep_url : "NULL");
    LOG_INFO("Term Url: %s", client_data.term_url ? client_data.term_url : "NULL");
    LOG_INFO("下一次重试: %s 秒后", client_data.keep_retry ? client_data.keep_retry : "NULL");
    return AUTH_SUCCESS;
}

static AuthStatus getTicket()
{
    char* payload = createXMLPayload(GET_TICKET);
    if (!payload)
    {
        LOG_ERROR("创建获取 Ticket XML 失败");
        return AUTH_FAILURE;
    }
    char* encrypt = sessionEncrypt(payload);
    free(payload);
    if (!encrypt)
    {
        LOG_ERROR("加密获取 Ticket XML 失败");
        return AUTH_FAILURE;
    }
    LOG_DEBUG("发送加密获取 ticket 内容: %s", encrypt);
    HTTPResponse* result = simPost(dialer_adapter.auth_config.auth_url, encrypt);
    free(encrypt);
    if (!result || result->status == REQUEST_ERROR)
    {
        LOG_ERROR("获取 Ticket 响应失败，错误代码: %d", result->status);
        freeResult(result);
        return AUTH_FAILURE;
    }
    LOG_DEBUG("获取 Ticket 响应内容: %s", result->data);
    char* decrypt = sessionDecrypt(result->data);
    freeResult(result);
    if (!decrypt)
    {
        LOG_ERROR("解密 Ticket 内容失败");
        return AUTH_FAILURE;
    }
    char* parsed_ticket = XmlParser(decrypt, "ticket");
    free(decrypt);
    if (!parsed_ticket)
    {
        LOG_ERROR("解析 Ticket 失败");
        return AUTH_FAILURE;
    }
    char* new_ticket = strdup(parsed_ticket);
    free(parsed_ticket);
    if (!new_ticket)
    {
        LOG_ERROR("复制新 Ticket 失败");
        return AUTH_FAILURE;
    }
    if (dialer_adapter.auth_config.ticket) free(dialer_adapter.auth_config.ticket);
    dialer_adapter.auth_config.ticket = new_ticket;
    return AUTH_SUCCESS;
}

static AuthStatus initSession()
{
    HTTPResponse* result = simPost(dialer_adapter.auth_config.ticket_url, dialer_adapter.auth_config.algo_id);
    if (!result || result->status == REQUEST_ERROR)
    {
        LOG_ERROR("初始化会话失败，错误代码: %d", result->status);
        freeResult(result);
        return AUTH_FAILURE;
    }
    LOG_DEBUG("会话响应内容: %s", result->data);
    const ByteArray zsm = stringToBytes(result->data);
    freeResult(result);
    initialize(zsm);
    free(zsm.data);
    return AUTH_SUCCESS;
}

static RunningStatus authorization()
{
    if (initSession() == AUTH_FAILURE || getTicket() == AUTH_FAILURE || login() == AUTH_FAILURE)
    {
        if (initSession() == AUTH_FAILURE) LOG_FATAL("初始化会话失败");
        else if (getTicket() == AUTH_FAILURE) LOG_FATAL("获取 Ticket 失败");
        else if (login() == AUTH_FAILURE) LOG_FATAL("登录失败");
        freeSession();
        dialer_adapter.runtime_status.is_running = false;
        return RUNNING_FAILURE;
    }
    LOG_INFO("Client IP: %s", dialer_adapter.auth_config.user_ip);
    LOG_INFO("AC IP: %s", dialer_adapter.auth_config.ac_ip);
    LOG_INFO("Ticket: %s", dialer_adapter.auth_config.ticket);
    client_data.tick = currentTimeMillis();
    dialer_adapter.timestamp.auth_time = currentTimeMillis();
    LOG_DEBUG("登录时间戳 (毫秒): %lld", dialer_adapter.timestamp.auth_time);
    dialer_adapter.runtime_status.is_logged = true;
    LOG_INFO("已认证登录");
    return RUNNING_SUCCESS;
}

RunningStatus run()
{
    switch (checkNetworkStatus())
    {
    case REQUEST_SUCCESS:
        if (dialer_adapter.runtime_status.is_initialized && dialer_adapter.runtime_status.is_logged)
        {
            const long long keep_retry = stringToLongLong(client_data.keep_retry);
            if (keep_retry != 0)
            {
                if (currentTimeMillis() - client_data.tick >= keep_retry * 1000)
                {
                    LOG_INFO("发送心跳包");
                    if (heartbeat() == RUNNING_FAILURE)
                    {
                        LOG_ERROR("心跳包发送失败");
                        return RUNNING_FAILURE;
                    }
                    LOG_INFO("下一次重试: %s 秒后", client_data.keep_retry);
                    client_data.tick = currentTimeMillis();
                }
            }
        }
        else
            LOG_INFO("网络已连接");
        sleepMilliseconds(1000);
        return RUNNING_SUCCESS;
    case REQUEST_AUTHORIZATION:
        LOG_INFO("需要认证");
        if (authorization() == RUNNING_FAILURE)
        {
            LOG_FATAL("认证失败");
            return RUNNING_FAILURE;
        }
        sleepMilliseconds(1000);
        return RUNNING_SUCCESS;
    case REQUEST_ERROR:
        LOG_ERROR("网络错误");
        sleepMilliseconds(5000);
        return RUNNING_FAILURE;
    case INIT_ERROR:
        LOG_ERROR("初始化错误");
        sleepMilliseconds(5000);
        return RUNNING_FAILURE;
    default:
        LOG_ERROR("未知错误");
        sleepMilliseconds(5000);
        return RUNNING_FAILURE;
    }
}