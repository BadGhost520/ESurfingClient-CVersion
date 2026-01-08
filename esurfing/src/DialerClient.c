#include <stdlib.h>

#include "headFiles/cipher/CipherInterface.h"
#include "headFiles/utils/PlatformUtils.h"
#include "headFiles/utils/Shutdown.h"
#include "headFiles/utils/Logger.h"
#include "headFiles/DialerClient.h"
#include "headFiles/NetClient.h"
#include "headFiles/Session.h"
#include "headFiles/States.h"

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
    const HTTPResponse result = sessionPost(client_data.term_url, encrypt);
    free(encrypt);
    if (result.status == REQUEST_ERROR)
    {
        LOG_ERROR("登出错误，错误代码: %d", result.status);
        free(result.body_data);
        return RUNNING_FAILURE;
    }
    free(result.body_data);
    thread_status[thread_index].dialer_context.runtime_status.is_authed = 0;
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
    const HTTPResponse result = sessionPost(client_data.keep_url, encrypt);
    free(encrypt);
    if (result.status == REQUEST_ERROR)
    {
        LOG_ERROR("心跳响应失败，错误代码: %d", result.status);
        free(result.body_data);
        return RUNNING_FAILURE;
    }
    char* decrypted_data = sessionDecrypt(result.body_data);
    free(result.body_data);
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
    snprintf(client_data.keep_retry, KEEP_RETRY_LENGTH, "%s", parsed_interval);
    free(parsed_interval);
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
    const HTTPResponse result = sessionPost(thread_status[thread_index].dialer_context.auth_config.auth_url, encrypt);
    free(encrypt);
    if (result.status == REQUEST_ERROR)
    {
        LOG_ERROR("登录响应失败，错误代码: %d", result.status);
        free(result.body_data);
        return AUTH_FAILURE;
    }
    LOG_DEBUG("登录响应内容: %s", result.body_data);
    char* decrypted_data = sessionDecrypt(result.body_data);
    free(result.body_data);
    if (!decrypted_data)
    {
        LOG_ERROR("解密登录响应内容失败");
        return AUTH_FAILURE;
    }
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
    snprintf(client_data.keep_url, KEEP_URL_LENGTH, "%s", cleaned_keep_url);
    free(cleaned_keep_url);
    LOG_INFO("Keep-Url: %s", client_data.keep_url);
    char* parsed_term_url = XmlParser(decrypted_data, "term-url");
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
    snprintf(client_data.term_url, TERM_URL_LENGTH, "%s", cleaned_term_url);
    free(cleaned_term_url);
    LOG_INFO("Term-Url: %s", client_data.term_url);
    char* parsed_interval = XmlParser(decrypted_data, "keep-retry");
    free(decrypted_data);
    if (!parsed_interval)
    {
        LOG_ERROR("解析 KeepRetry 失败");
        return AUTH_FAILURE;
    }
    snprintf(client_data.keep_retry, KEEP_RETRY_LENGTH, "%s", parsed_interval);
    free(parsed_interval);
    LOG_INFO("下一次重试: %s 秒后", client_data.keep_retry);
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
    const HTTPResponse result = sessionPost(thread_status[thread_index].dialer_context.auth_config.ticket_url, encrypt);
    free(encrypt);
    if (result.status == REQUEST_ERROR)
    {
        LOG_ERROR("获取 Ticket 响应失败，错误代码: %d", result.status);
        free(result.body_data);
        return AUTH_FAILURE;
    }
    LOG_DEBUG("获取 Ticket 响应内容: %s", result.body_data);
    char* decrypt = sessionDecrypt(result.body_data);
    free(result.body_data);
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
    snprintf(thread_status[thread_index].dialer_context.auth_config.ticket, TICKET_LENGTH, "%s", parsed_ticket);
    return AUTH_SUCCESS;
}

static AuthStatus initSession()
{
    const HTTPResponse result = sessionPost(thread_status[thread_index].dialer_context.auth_config.ticket_url, thread_status[thread_index].dialer_context.auth_config.algo_id);
    if (result.status == REQUEST_ERROR)
    {
        LOG_ERROR("初始化会话失败，错误代码: %d", result.status);
        free(result.body_data);
        return AUTH_FAILURE;
    }
    LOG_DEBUG("会话响应内容: %s", result.body_data);
    const ByteArray zsm = stringToBytes(result.body_data);
    free(result.body_data);
    initialize(zsm);
    free(zsm.data);
    return AUTH_SUCCESS;
}

static RunningStatus authorization()
{
    if (initSession() == AUTH_FAILURE)
    {
        LOG_FATAL("初始化会话失败");
        freeSession();
        thread_status[thread_index].dialer_context.runtime_status.is_running = false;
        return RUNNING_FAILURE;
    }
    LOG_DEBUG("初始化会话完成");
    if (getTicket() == AUTH_FAILURE)
    {
        LOG_FATAL("获取 Ticket 失败");
        freeSession();
        thread_status[thread_index].dialer_context.runtime_status.is_running = false;
        return RUNNING_FAILURE;
    }
    LOG_DEBUG("完成获取 Ticket");
    LOG_INFO("Ticket: %s", thread_status[thread_index].dialer_context.auth_config.ticket);
    if (login() == AUTH_FAILURE)
    {
        LOG_FATAL("登录失败");
        freeSession();
        thread_status[thread_index].dialer_context.runtime_status.is_running = false;
        return RUNNING_FAILURE;
    }
    LOG_DEBUG("完成登录");
    client_data.tick = currentTimeMillis();
    thread_status[thread_index].dialer_context.auth_time = currentTimeMillis();
    LOG_DEBUG("登录时间戳 (毫秒): %lld", thread_status[thread_index].dialer_context.auth_time);
    thread_status[thread_index].dialer_context.runtime_status.is_authed = true;
    LOG_INFO("已认证登录");
    return RUNNING_SUCCESS;
}

static RunningStatus run()
{
    switch (checkAuthStatus())
    {
    case REQUEST_SUCCESS:
        if (thread_status[thread_index].dialer_context.runtime_status.is_initialized && thread_status[thread_index].dialer_context.runtime_status.is_authed)
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
        else{
            LOG_INFO("网络已连接");
        }
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
    case REQUEST_REDIRECT:
        LOG_WARN("需要重定向");
        sleepMilliseconds(1000);
        return RUNNING_WARNING;
    case REQUEST_WARNING:
        LOG_WARN("网络超时");
        sleepMilliseconds(5000);
        return RUNNING_WARNING;
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

static void restart()
{
    if (thread_status[thread_index].dialer_context.runtime_status.is_initialized)
    {
        if (thread_status[thread_index].dialer_context.runtime_status.is_authed) term();
        freeSession();
    }
    thread_status[thread_index].dialer_context.auth_time = 0;
    sleepMilliseconds(5000);
    refreshStates();
}

void* dialerApp(void* arg)
{
    thread_index = (int)(intptr_t)arg;
    thread_status[thread_index].dialer_context.runtime_status.is_running = 1;
    refreshStates();
    LOG_INFO("认证线程启动中，序号: %d", thread_index + 1);
    sleepMilliseconds(5000);
    while (thread_status[thread_index].dialer_context.runtime_status.is_running)
    {
        if (currentTimeMillis() - thread_status[thread_index].dialer_context.auth_time >= 172200000 && thread_status[thread_index].dialer_context.auth_time != 0)
        {
            if (thread_status[thread_index].dialer_context.runtime_status.is_settings_changed)
            {
                LOG_INFO("设置已更改，正在重启认证");
                thread_status[thread_index].dialer_context.runtime_status.is_settings_changed = false;
            }
            LOG_DEBUG("当前时间戳(毫秒): %lld", currentTimeMillis());
            LOG_WARN("已登录 2870 分钟(1 天 23 小时 50 分钟)，为避免被远程服务器踢下线，正在重新进行认证");
            restart();
        }
        else if (thread_status[thread_index].dialer_context.runtime_status.is_settings_changed)
        {
            LOG_INFO("设置已更改，正在重启认证");
            restart();
            thread_status[thread_index].dialer_context.runtime_status.is_settings_changed = false;
        }
        if (run() == RUNNING_FAILURE) thread_status[thread_index].need_stop = true;
        checkAdapterStop();
    }
    return NULL;
}
