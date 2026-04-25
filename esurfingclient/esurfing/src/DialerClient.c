#include "cipher/CipherInterface.h"
#include "utils/PlatformUtils.h"
#include "utils/Logger.h"
#include "DialerClient.h"
#include "NetClient.h"
#include "States.h"

#include <stdlib.h>
#include <string.h>

static RunningStatus term()
{
    char* payload = create_xml_payload(TERM);
    if (!payload)
    {
        LOG_ERROR("登出 XML 创建失败");
        return RUNNING_FAILURE;
    }
    char* encrypt = session_encrypt(payload);
    if (!encrypt)
    {
        LOG_ERROR("登出 XML 加密失败");
        return RUNNING_FAILURE;
    }
    LOG_VERBOSE("发送加密登出内容: %s", encrypt);
    const HTTPResponse result = post(g_prog_status[thread_idx].auth_cfg.term_url, encrypt);
    free(encrypt);
    if (result.status != REQUEST_SUCCESS)
    {
        LOG_ERROR("登出错误，错误代码: %d", result.status);
        free(result.body_data);
        return RUNNING_FAILURE;
    }
    free(result.body_data);
    g_prog_status[thread_idx].runtime_status.is_authed = false;
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
    char* encrypt = session_encrypt(payload);
    if (!encrypt)
    {
        LOG_ERROR("加密心跳 XML 失败");
        return RUNNING_FAILURE;
    }
    LOG_VERBOSE("发送加密心跳内容: %s", encrypt);
    const HTTPResponse result = post(g_prog_status[thread_idx].auth_cfg.keep_url, encrypt);
    free(encrypt);
    if (result.status == REQUEST_ERROR)
    {
        LOG_ERROR("心跳响应失败，错误代码: %d", result.status);
        free(result.body_data);
        return RUNNING_FAILURE;
    }
    char* decrypted_data = session_decrypt(result.body_data);
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
    g_prog_status[thread_idx].auth_cfg.keep_retry = str_2_uint64(parsed_interval);
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
    char* encrypt = session_encrypt(payload);
    if (!encrypt)
    {
        LOG_ERROR("加密登录 XML 失败");
        return AUTH_FAILURE;
    }
    LOG_VERBOSE("发送加密登录内容: %s", encrypt);
    const HTTPResponse result = post(g_prog_status[thread_idx].auth_cfg.auth_url, encrypt);
    free(encrypt);
    if (result.status == REQUEST_ERROR)
    {
        LOG_ERROR("登录响应失败，错误代码: %d", result.status);
        free(result.body_data);
        return AUTH_FAILURE;
    }
    LOG_VERBOSE("登录响应内容: %s", result.body_data);
    char* decrypted_data = session_decrypt(result.body_data);
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
    snprintf(g_prog_status[thread_idx].auth_cfg.keep_url, KEEP_URL_LEN, "%s", cleaned_keep_url);
    free(cleaned_keep_url);
    LOG_INFO("Keep-Url: %s", g_prog_status[thread_idx].auth_cfg.keep_url);
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
    snprintf(g_prog_status[thread_idx].auth_cfg.term_url, TERM_URL_LEN, "%s", cleaned_term_url);
    free(cleaned_term_url);
    LOG_INFO("Term-Url: %s", g_prog_status[thread_idx].auth_cfg.term_url);
    char* parsed_interval = xml_parser(decrypted_data, "keep-retry");
    free(decrypted_data);
    if (!parsed_interval)
    {
        LOG_ERROR("解析 KeepRetry 失败");
        return AUTH_FAILURE;
    }
    g_prog_status[thread_idx].auth_cfg.keep_retry = str_2_uint64(parsed_interval);
    free(parsed_interval);
    LOG_INFO("下一次重试: %" PRIu64 " 秒后", g_prog_status[thread_idx].auth_cfg.keep_retry);
    return AUTH_SUCCESS;
}

static AuthStatus get_ticket()
{
    LOG_DEBUG("get_ticket 函数入口检查, 使用配置: %" PRIu8 ", 下标: %" PRIu8, g_prog_status[thread_idx].login_cfg.idx, thread_idx);
    char* payload = create_xml_payload(GET_TICKET);
    if (!payload)
    {
        LOG_ERROR("创建获取 Ticket XML 失败");
        return AUTH_FAILURE;
    }
    char* encrypt = session_encrypt(payload);
    if (!encrypt)
    {
        LOG_ERROR("加密获取 Ticket XML 失败");
        return AUTH_FAILURE;
    }
    LOG_VERBOSE("发送加密获取 ticket 内容: %s", encrypt);
    const HTTPResponse result = post(g_prog_status[thread_idx].auth_cfg.ticket_url, encrypt);
    free(encrypt);
    if (result.status == REQUEST_ERROR)
    {
        LOG_ERROR("获取 Ticket 响应失败，错误代码: %d", result.status);
        free(result.body_data);
        return AUTH_FAILURE;
    }
    LOG_VERBOSE("获取 Ticket 响应内容: %s", result.body_data);
    char* decrypt = session_decrypt(result.body_data);
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
    snprintf(g_prog_status[thread_idx].auth_cfg.ticket, TICKET_LEN, "%s", parsed_ticket);
    return AUTH_SUCCESS;
}

static InitStatus load(const ByteArray zsm)
{
    LOG_DEBUG("load 函数入口检查, 使用配置: %" PRIu8 ", 下标: %" PRIu8, g_prog_status[thread_idx].login_cfg.idx, thread_idx);
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
    algo_id[ALGO_ID_LEN - 1] = '\0';
    LOG_INFO("Algo ID: %s", algo_id);
    if (!init_cipher(algo_id))
    {
        LOG_ERROR("初始化加解密工厂失败");
        return INIT_FAILURE;
    }
    LOG_DEBUG("全局 AlgoID 已更新: '%s'", algo_id);
    snprintf(g_prog_status[thread_idx].auth_cfg.algo_id, ALGO_ID_LEN, "%s", algo_id);
    return INIT_SUCCESS;
}

static void clean_session()
{
    LOG_DEBUG("清除会话初始化状态");
    destroy_cipher_factory();
    g_prog_status[thread_idx].runtime_status.is_initialized = 0;
}

static AuthStatus init_session()
{
    LOG_DEBUG("init_session 函数入口检查, 使用配置: %" PRIu8 ", 下标: %" PRIu8, g_prog_status[thread_idx].login_cfg.idx, thread_idx);
    const HTTPResponse result = post(g_prog_status[thread_idx].auth_cfg.ticket_url, g_prog_status[thread_idx].auth_cfg.algo_id);
    if (result.status == REQUEST_ERROR)
    {
        LOG_ERROR("初始化会话失败，错误代码: %d", result.status);
        free(result.body_data);
        return AUTH_FAILURE;
    }
    LOG_VERBOSE("会话响应内容: %s", result.body_data);
    const ByteArray zsm = str_2_bytes(result.body_data);
    free(result.body_data);

    LOG_DEBUG("开始初始化会话");
    if (load(zsm) == INIT_SUCCESS)
    {
        LOG_DEBUG("初始化会话成功");
        g_prog_status[thread_idx].runtime_status.is_initialized = 1;
        free(zsm.data);
        return AUTH_SUCCESS;
    }
    LOG_DEBUG("初始化会话失败");
    g_prog_status[thread_idx].runtime_status.is_initialized = 0;
    free(zsm.data);
    return AUTH_FAILURE;
}

static RunningStatus auth()
{
    LOG_DEBUG("auth 函数入口检查, 使用配置: %" PRIu8 ", 下标: %" PRId8, g_prog_status[thread_idx].login_cfg.idx, thread_idx);

    const char portal_start_tag[] = "<!--//config.campus.js.chinatelecom.com";
    const char portal_end_tag[] = "//config.campus.js.chinatelecom.com-->";

    const HTTPResponse resp = get(g_prog_status[thread_idx].last_location);
    if (resp.status != REQUEST_HAVE_RES)
    {
        LOG_ERROR("响应体为空, 无法提取认证配置");
        return RUNNING_FAILURE;
    }

    char* portal_config = extract_between_tags(resp.body_data, portal_start_tag, portal_end_tag);
    free(resp.body_data);
    if (!portal_config)
    {
        LOG_ERROR("提取门户配置失败");
        return RUNNING_FAILURE;
    }

    char* auth_url = xml_parser(portal_config, "auth-url");
    if (!auth_url)
    {
        LOG_ERROR("提取 Auth URL 失败");
        return RUNNING_FAILURE;
    }

    char* cleaned_auth_url = clean_CDATA(auth_url);
    free(auth_url);
    if (!cleaned_auth_url)
    {
        LOG_ERROR("清除 Auth URL 失败");
        return RUNNING_FAILURE;
    }
    LOG_INFO("Auth URL: %s", cleaned_auth_url);
    snprintf(g_prog_status[thread_idx].auth_cfg.auth_url, AUTH_URL_LEN, "%s", cleaned_auth_url);
    free(cleaned_auth_url);

    char* ticket_url = xml_parser(portal_config, "ticket-url");
    free(portal_config);
    if (!ticket_url)
    {
        LOG_ERROR("提取 Ticket URL 失败");
        return RUNNING_FAILURE;
    }

    char* cleaned_ticket_url = clean_CDATA(ticket_url);
    free(ticket_url);
    if (!cleaned_ticket_url)
    {
        LOG_ERROR("清除 Ticket URL CDATA 失败");
        return RUNNING_FAILURE;
    }
    LOG_INFO("Ticket URL: %s", cleaned_ticket_url);
    snprintf(g_prog_status[thread_idx].auth_cfg.ticket_url, TICKET_URL_LEN, "%s", cleaned_ticket_url);

    char* client_ip = extract_url_param(cleaned_ticket_url, "wlanuserip");
    if (!client_ip)
    {
        LOG_ERROR("提取 Client IP 失败");
        return RUNNING_FAILURE;
    }
    LOG_INFO("Client IP: %s", client_ip);
    snprintf(g_prog_status[thread_idx].auth_cfg.client_ip, IP_LEN, "%s", client_ip);
    free(client_ip);

    char* ac_ip = extract_url_param(cleaned_ticket_url, "wlanacip");
    free(cleaned_ticket_url);
    if (!ac_ip)
    {
        LOG_ERROR("提取 AC IP 失败");
        return RUNNING_FAILURE;
    }
    LOG_INFO("AC IP: %s", ac_ip);
    snprintf(g_prog_status[thread_idx].auth_cfg.ac_ip, IP_LEN, "%s", ac_ip);
    free(ac_ip);

    if (init_session() == AUTH_FAILURE)
    {
        LOG_FATAL("初始化会话失败");
        clean_session();
        g_prog_status[thread_idx].runtime_status.is_running = false;
        return RUNNING_FAILURE;
    }
    LOG_DEBUG("初始化会话完成");
    if (get_ticket() == AUTH_FAILURE)
    {
        LOG_FATAL("获取 Ticket 失败");
        clean_session();
        g_prog_status[thread_idx].runtime_status.is_running = false;
        return RUNNING_FAILURE;
    }
    LOG_DEBUG("完成获取 Ticket");
    LOG_INFO("Ticket: %s", g_prog_status[thread_idx].auth_cfg.ticket);
    if (login() == AUTH_FAILURE)
    {
        LOG_FATAL("登录失败");
        clean_session();
        g_prog_status[thread_idx].runtime_status.is_running = false;
        return RUNNING_FAILURE;
    }
    LOG_DEBUG("完成登录");
    g_prog_status[thread_idx].auth_cfg.tick = get_cur_tm_ms();
    g_prog_status[thread_idx].auth_cfg.auth_time = get_cur_tm_ms();
    LOG_DEBUG("登录时间戳 (毫秒): %" PRIu64, g_prog_status[thread_idx].auth_cfg.auth_time);
    g_prog_status[thread_idx].runtime_status.is_authed = true;
    LOG_INFO("已认证登录");
    return RUNNING_SUCCESS;
}

static RunningStatus run()
{
    switch (check_network_status())
    {
    case REQUEST_SUCCESS:
        ;
        bool have_auth = false;
        for (uint8_t i = 0; i < g_prog_cnt; i++)
        {
            if (g_prog_status[thread_idx].runtime_status.is_authed) have_auth = true;
        }
        if (have_auth)
        {
            if (!g_prog_status[thread_idx].runtime_status.is_authed) auth();
        }

        if (g_prog_status[thread_idx].runtime_status.is_initialized && g_prog_status[thread_idx].runtime_status.is_authed)
        {
            if (g_prog_status[thread_idx].auth_cfg.keep_retry != 0)
            {
                if (get_cur_tm_ms() - g_prog_status[thread_idx].auth_cfg.tick >= g_prog_status[thread_idx].auth_cfg.keep_retry * 1000)
                {
                    LOG_INFO("发送心跳包");
                    if (heartbeat() == RUNNING_FAILURE)
                    {
                        LOG_ERROR("心跳包发送失败");
                        return RUNNING_FAILURE;
                    }
                    LOG_INFO("下一次重试: %" PRIu64 " 秒后", g_prog_status[thread_idx].auth_cfg.keep_retry);
                    g_prog_status[thread_idx].auth_cfg.tick = get_cur_tm_ms();
                }
            }
        }
        else
        {
            LOG_INFO("已连接到互联网");
        }
        sleep_ms(1000);
        return RUNNING_SUCCESS;
    case REQUEST_REDIRECT:
        LOG_INFO("需要认证");
        if (auth() == RUNNING_FAILURE)
        {
            LOG_FATAL("认证失败");
            sleep_ms(5000);
            return RUNNING_FAILURE;
        }
        sleep_ms(1000);
        return RUNNING_SUCCESS;
    default:
        LOG_ERROR("未知错误");
        sleep_ms(5000);
        return RUNNING_FAILURE;
    }
}

void reset()
{
    if (g_prog_status[thread_idx].runtime_status.is_initialized)
    {
        if (g_prog_status[thread_idx].runtime_status.is_authed) term();
        clean_session();
    }
    memset(&g_prog_status[thread_idx].auth_cfg, 0, sizeof(AuthConfig));
    g_prog_status[thread_idx].runtime_status.is_running = false;
    g_prog_status[thread_idx].runtime_status.is_need_reset = false;
    g_prog_status[thread_idx].runtime_status.last_location_lock = false;
    refresh_states();
    get_last_location();
}

int dialer_app(void* arg)
{
    thread_idx = (int8_t)(intptr_t)arg;
    g_prog_status[thread_idx].thread_id = sim_thread_cur_id();
    LOG_DEBUG("线程 %" PRId8 " 创建成功, ID: %" PRIu64, thread_idx, g_prog_status[thread_idx].thread_id);
    g_prog_status[thread_idx].runtime_status.is_running = true;
    while (g_prog_status[thread_idx].runtime_status.is_running)
    {
        if (run() == RUNNING_FAILURE) g_prog_status[thread_idx].runtime_status.is_running = false;
        if (g_prog_status[thread_idx].runtime_status.is_need_reset) reset();
    }
    if (g_prog_status[thread_idx].runtime_status.is_authed)
    {
        LOG_DEBUG("配置 %" PRIu8 " 登出, 下标: %" PRId8, g_prog_status[thread_idx].login_cfg.idx, thread_idx);
        term();
        clean_session();
    }
    return 0;
}
