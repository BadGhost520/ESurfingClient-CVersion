#include "cipher/CipherInterface.h"
#include "utils/PlatformUtils.h"
#include "utils/Logger.h"
#include "DialerClient.h"
#include "NetClient.h"
#include "States.h"

#include <stdlib.h>
#include <string.h>

static InitStatus load(const ByteArray zsm)
{
    LOG_DEBUG("接收到的 zsm 数据长度: %zu", zsm.length);
    if (!zsm.data || zsm.length == 0)
    {
        LOG_ERROR("无效的 zsm 数据");
        return INIT_FAILURE;
    }
    char str[zsm.length + 1];
    memcpy(str, zsm.data, zsm.length);
    str[zsm.length] = '\0';
    const size_t length = strlen(str);
    LOG_DEBUG("原始字符串: %s", str);
    LOG_DEBUG("字符串长度: %zu", length);
    if (length < 4 + 38)
    {
        LOG_ERROR("字符串长度不足");
        return INIT_FAILURE;
    }
    char algo_id[ALGO_ID_LEN];
    memcpy(algo_id, str + length - 37, ALGO_ID_LEN - 1);
    LOG_INFO("Algo ID: %s", algo_id);
    if (!initCipher(algo_id))
    {
        LOG_ERROR("初始化加解密工厂失败");
        return INIT_FAILURE;
    }
    LOG_DEBUG("全局 AlgoID 已更新: '%s'", algo_id);
    snprintf(g_prog_status[g_prog_idx].auth_cfg.algo_id, ALGO_ID_LEN, "%s", algo_id);
    return INIT_SUCCESS;
}

static AuthStatus initSession()
{
    const HTTPResponse result = post_with_header(g_prog_status[g_prog_idx].auth_cfg.ticket_url, g_prog_status[g_prog_idx].auth_cfg.algo_id);
    if (result.status == REQUEST_ERROR)
    {
        LOG_ERROR("初始化会话失败，错误代码: %d", result.status);
        free(result.body_data);
        return AUTH_FAILURE;
    }
    LOG_DEBUG("会话响应内容: %s", result.body_data);
    const ByteArray zsm = str_2_bytes(result.body_data);
    free(result.body_data);

    LOG_DEBUG("开始初始化会话");
    if (load(zsm) == INIT_SUCCESS)
    {
        LOG_DEBUG("初始化会话成功");
        g_prog_status[g_prog_idx].runtime_status.is_initialized = 1;
    }
    else
    {
        LOG_DEBUG("初始化会话失败");
        g_prog_status[g_prog_idx].runtime_status.is_initialized = 0;
    }

    free(zsm.data);
    return AUTH_SUCCESS;
}

static void clean_session()
{
    LOG_DEBUG("清除会话初始化状态");
    cipherFactoryDestroy();
    g_prog_status[g_prog_idx].runtime_status.is_initialized = 0;
}

RunningStatus term()
{
    char* payload = create_xml_payload(TERM);
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
    LOG_VERBOSE("发送加密登出内容: %s", encrypt);
    const HTTPResponse result = post_with_header(g_prog_status[g_prog_idx].auth_cfg.term_url, encrypt);
    free(encrypt);
    if (result.status != REQUEST_SUCCESS)
    {
        LOG_ERROR("登出错误，错误代码: %d", result.status);
        free(result.body_data);
        return RUNNING_FAILURE;
    }
    free(result.body_data);
    g_prog_status[g_prog_idx].runtime_status.is_authed = false;
    return RUNNING_SUCCESS;
}

static RunningStatus heartbeat()
{
    char* payload = create_xml_payload(HEART_BEAT);
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
    LOG_VERBOSE("发送加密心跳内容: %s", encrypt);
    const HTTPResponse result = post_with_header(g_prog_status[g_prog_idx].auth_cfg.keep_url, encrypt);
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
    LOG_VERBOSE("心跳响应内容: %s", decrypted_data);
    char* parsed_interval = xml_parser(decrypted_data, "interval");
    free(decrypted_data);
    if (!parsed_interval)
    {
        LOG_ERROR("心跳内容解析失败");
        return RUNNING_FAILURE;
    }
    snprintf(g_prog_status[g_prog_idx].auth_cfg.keep_retry, KEEP_RETRY_LEN, "%s", parsed_interval);
    free(parsed_interval);
    return RUNNING_SUCCESS;
}

static AuthStatus login()
{
    char* payload = create_xml_payload(LOGIN);
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
    LOG_VERBOSE("发送加密登录内容: %s", encrypt);
    const HTTPResponse result = post_with_header(g_prog_status[g_prog_idx].auth_cfg.auth_url, encrypt);
    free(encrypt);
    if (result.status == REQUEST_ERROR)
    {
        LOG_ERROR("登录响应失败，错误代码: %d", result.status);
        free(result.body_data);
        return AUTH_FAILURE;
    }
    LOG_VERBOSE("登录响应内容: %s", result.body_data);
    char* decrypted_data = sessionDecrypt(result.body_data);
    free(result.body_data);
    if (!decrypted_data)
    {
        LOG_ERROR("解密登录响应内容失败");
        return AUTH_FAILURE;
    }
    char* parsed_keep_url = xml_parser(decrypted_data, "keep-url");
    if (!parsed_keep_url)
    {
        LOG_ERROR("解析 KeepURL 失败");
        return AUTH_FAILURE;
    }
    char* cleaned_keep_url = clean_CDATA(parsed_keep_url);
    free(parsed_keep_url);
    if (!cleaned_keep_url)
    {
        LOG_ERROR("清除 KeepURL CDATA 失败");
        return AUTH_FAILURE;
    }
    snprintf(g_prog_status[g_prog_idx].auth_cfg.keep_url, KEEP_URL_LEN, "%s", cleaned_keep_url);
    free(cleaned_keep_url);
    LOG_INFO("Keep-Url: %s", g_prog_status[g_prog_idx].auth_cfg.keep_url);
    char* parsed_term_url = xml_parser(decrypted_data, "term-url");
    if (!parsed_term_url)
    {
        LOG_ERROR("解析 TermURL 失败");
        return AUTH_FAILURE;
    }
    char* cleaned_term_url = clean_CDATA(parsed_term_url);
    free(parsed_term_url);
    if (!cleaned_term_url)
    {
        LOG_ERROR("清除 TermURL CDATA 失败");
        return AUTH_FAILURE;
    }
    snprintf(g_prog_status[g_prog_idx].auth_cfg.term_url, TERM_URL_LEN, "%s", cleaned_term_url);
    free(cleaned_term_url);
    LOG_INFO("Term-Url: %s", g_prog_status[g_prog_idx].auth_cfg.term_url);
    char* parsed_interval = xml_parser(decrypted_data, "keep-retry");
    free(decrypted_data);
    if (!parsed_interval)
    {
        LOG_ERROR("解析 KeepRetry 失败");
        return AUTH_FAILURE;
    }
    snprintf(g_prog_status[g_prog_idx].auth_cfg.keep_retry, KEEP_RETRY_LEN, "%s", parsed_interval);
    free(parsed_interval);
    LOG_INFO("下一次重试: %s 秒后", g_prog_status[g_prog_idx].auth_cfg.keep_retry);
    return AUTH_SUCCESS;
}

static AuthStatus get_ticket()
{
    char* payload = create_xml_payload(GET_TICKET);
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
    LOG_VERBOSE("发送加密获取 ticket 内容: %s", encrypt);
    const HTTPResponse result = post_with_header(g_prog_status[g_prog_idx].auth_cfg.ticket_url, encrypt);
    free(encrypt);
    if (result.status == REQUEST_ERROR)
    {
        LOG_ERROR("获取 Ticket 响应失败，错误代码: %d", result.status);
        free(result.body_data);
        return AUTH_FAILURE;
    }
    LOG_VERBOSE("获取 Ticket 响应内容: %s", result.body_data);
    char* decrypt = sessionDecrypt(result.body_data);
    free(result.body_data);
    if (!decrypt)
    {
        LOG_ERROR("解密 Ticket 内容失败");
        return AUTH_FAILURE;
    }
    char* parsed_ticket = xml_parser(decrypt, "ticket");
    free(decrypt);
    if (!parsed_ticket)
    {
        LOG_ERROR("解析 Ticket 失败");
        return AUTH_FAILURE;
    }
    snprintf(g_prog_status[g_prog_idx].auth_cfg.ticket, TICKET_LEN, "%s", parsed_ticket);
    return AUTH_SUCCESS;
}

static RunningStatus auth()
{
    if (initSession() == AUTH_FAILURE)
    {
        LOG_FATAL("初始化会话失败");
        clean_session();
        g_prog_status[g_prog_idx].runtime_status.is_running = false;
        return RUNNING_FAILURE;
    }
    LOG_DEBUG("初始化会话完成");
    if (get_ticket() == AUTH_FAILURE)
    {
        LOG_FATAL("获取 Ticket 失败");
        clean_session();
        g_prog_status[g_prog_idx].runtime_status.is_running = false;
        return RUNNING_FAILURE;
    }
    LOG_DEBUG("完成获取 Ticket");
    LOG_INFO("Ticket: %s", g_prog_status[g_prog_idx].auth_cfg.ticket);
    if (login() == AUTH_FAILURE)
    {
        LOG_FATAL("登录失败");
        clean_session();
        g_prog_status[g_prog_idx].runtime_status.is_running = false;
        return RUNNING_FAILURE;
    }
    LOG_DEBUG("完成登录");
    g_prog_status[g_prog_idx].auth_cfg.tick = cur_tm_ms();
    g_prog_status[g_prog_idx].runtime_status.auth_time = cur_tm_ms();
    LOG_DEBUG("登录时间戳 (毫秒): %lld", g_prog_status[g_prog_idx].runtime_status.auth_time);
    g_prog_status[g_prog_idx].runtime_status.is_authed = true;
    LOG_INFO("已认证登录");
    return RUNNING_SUCCESS;
}

static RunningStatus run()
{
    switch (check_auth_status())
    {
    case REQUEST_SUCCESS:
        if (g_prog_status[g_prog_idx].runtime_status.is_initialized && g_prog_status[g_prog_idx].runtime_status.is_authed)
        {
            const uint64_t keep_retry = str_2_uint64(g_prog_status[g_prog_idx].auth_cfg.keep_retry);
            if (keep_retry != 0)
            {
                if (cur_tm_ms() - g_prog_status[g_prog_idx].auth_cfg.tick >= keep_retry * 1000)
                {
                    LOG_INFO("发送心跳包");
                    if (heartbeat() == RUNNING_FAILURE)
                    {
                        LOG_ERROR("心跳包发送失败");
                        return RUNNING_FAILURE;
                    }
                    LOG_INFO("下一次重试: %s 秒后", g_prog_status[g_prog_idx].auth_cfg.keep_retry);
                    g_prog_status[g_prog_idx].auth_cfg.tick = cur_tm_ms();
                }
            }
        }
        else
            LOG_INFO("网络已连接");
        sleep_ms(1000);
        return RUNNING_SUCCESS;
    case REQUEST_AUTHORIZATION:
        LOG_INFO("需要认证");
        if (auth() == RUNNING_FAILURE)
        {
            LOG_FATAL("认证失败");
            return RUNNING_FAILURE;
        }
        sleep_ms(1000);
        return RUNNING_SUCCESS;
    case REQUEST_ERROR:
        LOG_ERROR("网络错误");
        sleep_ms(5000);
        return RUNNING_FAILURE;
    case REQUEST_REDIRECT:
        LOG_WARN("需要重定向");
        sleep_ms(1000);
        return RUNNING_WARNING;
    case REQUEST_WARNING:
        LOG_WARN("网络超时");
        sleep_ms(5000);
        return RUNNING_WARNING;
    case INIT_ERROR:
        LOG_ERROR("初始化错误");
        sleep_ms(5000);
        return RUNNING_FAILURE;
    default:
        LOG_ERROR("未知错误");
        sleep_ms(5000);
        return RUNNING_FAILURE;
    }
}

static void restart()
{
    if (g_prog_status[g_prog_idx].runtime_status.is_initialized)
    {
        if (g_prog_status[g_prog_idx].runtime_status.is_authed) term();
        clean_session();
    }
    memset(&g_prog_status[g_prog_idx].auth_cfg, 0, sizeof(AuthConfig));
    memset(&g_prog_status[g_prog_idx].runtime_status, 0, sizeof(RuntimeStatus));
    sleep_ms(5000);
    refresh_states();
}

void dialer_app()
{
    refresh_states();
    while (g_prog_status[g_prog_idx].runtime_status.is_running)
    {
        if (cur_tm_ms() - g_prog_status[g_prog_idx].runtime_status.auth_time >= 172200000 && g_prog_status[g_prog_idx].runtime_status.auth_time != 0)
        {
            if (g_prog_status[g_prog_idx].runtime_status.is_settings_changed)
            {
                LOG_INFO("设置已更改，正在重启认证");
                g_prog_status[g_prog_idx].runtime_status.is_settings_changed = false;
            }
            LOG_DEBUG("当前时间戳(毫秒): %lld", cur_tm_ms());
            LOG_WARN("已登录 2870 分钟(1 天 23 小时 50 分钟)，为避免被远程服务器踢下线，正在重新进行认证");
            restart();
        }
        else if (g_prog_status[g_prog_idx].runtime_status.is_settings_changed)
        {
            LOG_INFO("设置已更改，正在重启认证");
            restart();
            g_prog_status[g_prog_idx].runtime_status.is_settings_changed = false;
        }
        run();
        sleep_ms(1000);
    }
}
