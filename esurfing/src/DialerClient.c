#include "cipher/CipherInterface.h"
#include "utils/PlatformUtils.h"
#include "utils/Shutdown.h"
#include "utils/Logger.h"
#include "DialerClient.h"
#include "NetClient.h"
#include "Session.h"
#include "States.h"

#include <stdlib.h>
#include <string.h>

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
    const HTTPResponse result = sessionPost(prog_status[prog_index].auth_config.term_url, encrypt);
    free(encrypt);
    if (result.status != REQUEST_SUCCESS)
    {
        LOG_ERROR("登出错误，错误代码: %d", result.status);
        free(result.body_data);
        return RUNNING_FAILURE;
    }
    free(result.body_data);
    prog_status[prog_index].runtime_status.is_authed = false;
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
    const HTTPResponse result = sessionPost(prog_status[prog_index].auth_config.keep_url, encrypt);
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
    snprintf(prog_status[prog_index].auth_config.keep_retry, KEEP_RETRY_LENGTH, "%s", parsed_interval);
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
    const HTTPResponse result = sessionPost(prog_status[prog_index].auth_config.auth_url, encrypt);
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
    snprintf(prog_status[prog_index].auth_config.keep_url, KEEP_URL_LENGTH, "%s", cleaned_keep_url);
    free(cleaned_keep_url);
    LOG_INFO("Keep-Url: %s", prog_status[prog_index].auth_config.keep_url);
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
    snprintf(prog_status[prog_index].auth_config.term_url, TERM_URL_LENGTH, "%s", cleaned_term_url);
    free(cleaned_term_url);
    LOG_INFO("Term-Url: %s", prog_status[prog_index].auth_config.term_url);
    char* parsed_interval = XmlParser(decrypted_data, "keep-retry");
    free(decrypted_data);
    if (!parsed_interval)
    {
        LOG_ERROR("解析 KeepRetry 失败");
        return AUTH_FAILURE;
    }
    snprintf(prog_status[prog_index].auth_config.keep_retry, KEEP_RETRY_LENGTH, "%s", parsed_interval);
    free(parsed_interval);
    LOG_INFO("下一次重试: %s 秒后", prog_status[prog_index].auth_config.keep_retry);
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
    const HTTPResponse result = sessionPost(prog_status[prog_index].auth_config.ticket_url, encrypt);
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
    snprintf(prog_status[prog_index].auth_config.ticket, TICKET_LENGTH, "%s", parsed_ticket);
    return AUTH_SUCCESS;
}

static AuthStatus initSession()
{
    const HTTPResponse result = sessionPost(prog_status[prog_index].auth_config.ticket_url, prog_status[prog_index].auth_config.algo_id);
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
        prog_status[prog_index].runtime_status.is_running = false;
        return RUNNING_FAILURE;
    }
    LOG_DEBUG("初始化会话完成");
    if (getTicket() == AUTH_FAILURE)
    {
        LOG_FATAL("获取 Ticket 失败");
        freeSession();
        prog_status[prog_index].runtime_status.is_running = false;
        return RUNNING_FAILURE;
    }
    LOG_DEBUG("完成获取 Ticket");
    LOG_INFO("Ticket: %s", prog_status[prog_index].auth_config.ticket);
    if (login() == AUTH_FAILURE)
    {
        LOG_FATAL("登录失败");
        freeSession();
        prog_status[prog_index].runtime_status.is_running = false;
        return RUNNING_FAILURE;
    }
    LOG_DEBUG("完成登录");
    prog_status[prog_index].auth_config.tick = currentTimeMillis();
    prog_status[prog_index].runtime_status.auth_time = currentTimeMillis();
    LOG_DEBUG("登录时间戳 (毫秒): %lld", prog_status[prog_index].runtime_status.auth_time);
    prog_status[prog_index].runtime_status.is_authed = true;
    LOG_INFO("已认证登录");
    return RUNNING_SUCCESS;
}

static RunningStatus run()
{
    switch (checkAuthStatus())
    {
    case REQUEST_SUCCESS:
        if (prog_status[prog_index].runtime_status.is_initialized && prog_status[prog_index].runtime_status.is_authed)
        {
            const uint64_t keep_retry = stringToUint64(prog_status[prog_index].auth_config.keep_retry);
            if (keep_retry != 0)
            {
                if (currentTimeMillis() - prog_status[prog_index].auth_config.tick >= keep_retry * 1000)
                {
                    LOG_INFO("发送心跳包");
                    if (heartbeat() == RUNNING_FAILURE)
                    {
                        LOG_ERROR("心跳包发送失败");
                        return RUNNING_FAILURE;
                    }
                    LOG_INFO("下一次重试: %s 秒后", prog_status[prog_index].auth_config.keep_retry);
                    prog_status[prog_index].auth_config.tick = currentTimeMillis();
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
    if (prog_status[prog_index].runtime_status.is_initialized)
    {
        if (prog_status[prog_index].runtime_status.is_authed) term();
        freeSession();
    }
    memset(&prog_status[prog_index].auth_config, 0, sizeof(AuthConfig));
    memset(&prog_status[prog_index].runtime_status, 0, sizeof(RuntimeStatus));
    sleepMilliseconds(5000);
    refreshStates();
}

void dialerApp()
{
    refreshStates();
    while (prog_status[prog_index].runtime_status.is_running)
    {
        if (currentTimeMillis() - prog_status[prog_index].runtime_status.auth_time >= 172200000 && prog_status[prog_index].runtime_status.auth_time != 0)
        {
            if (prog_status[prog_index].runtime_status.is_settings_changed)
            {
                LOG_INFO("设置已更改，正在重启认证");
                prog_status[prog_index].runtime_status.is_settings_changed = false;
            }
            LOG_DEBUG("当前时间戳(毫秒): %lld", currentTimeMillis());
            LOG_WARN("已登录 2870 分钟(1 天 23 小时 50 分钟)，为避免被远程服务器踢下线，正在重新进行认证");
            restart();
        }
        else if (prog_status[prog_index].runtime_status.is_settings_changed)
        {
            LOG_INFO("设置已更改，正在重启认证");
            restart();
            prog_status[prog_index].runtime_status.is_settings_changed = false;
        }
        run();
        sleepMilliseconds(1000);
    }
}
