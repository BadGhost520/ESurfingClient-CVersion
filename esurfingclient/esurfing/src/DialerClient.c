#include "cipher/CipherInterface.h"
#include "utils/PlatformUtils.h"
#include "utils/Logger.h"
#include "DialerClient.h"
#include "NetClient.h"
#include "States.h"

#include <stdlib.h>
#include <string.h>

static bool term()
{
    const char* xml = create_xml_payload(TERM); // 创建 term 配置 xml
    if (xml == NULL)
    {
        LOG_ERROR("登出 XML 创建失败");
        return false;
    }

    char* encrypt = session_encrypt(xml); // 加密 xml
    if (encrypt == NULL)
    {
        LOG_ERROR("登出 XML 加密失败");
        return false;
    }
    LOG_VERBOSE("发送加密登出内容: %s", encrypt);

    http_resp_t result = post(g_prog_status[thread_idx].auth_cfg.term_url, encrypt); // 向 term_url 发送加密数据
    uint8_t retry = 1;
    while (result.status != REQUEST_SUCCESS && result.status != REQUEST_HAVE_RES)
    {
        if (retry > 5)
        {
            LOG_FATAL("超过最多重试次数, 返回");
            free(encrypt);
            if (result.body_data) free(result.body_data);
            return false;
        }
        LOG_ERROR("配置 %" PRIu8 " 登出失败, 下标 %" PRIu8 ", 错误代码: %d, 重试: 第 %" PRIu8 " 次, 最多 5 次", g_prog_status[thread_idx].login_cfg.idx, thread_idx, result.status, retry);
        retry++;
        sleep_ms(1000);
        result = post(g_prog_status[thread_idx].auth_cfg.term_url, encrypt); // 向 term_url 发送加密数据 (重试)
    }
    free(encrypt);
    if (result.body_data) free(result.body_data);

    g_prog_status[thread_idx].runtime_status.is_authed = false;
    g_prog_status[thread_idx].auth_cfg.auth_time = 0;
    return true;
}

static bool heartbeat()
{
    const char* xml = create_xml_payload(HEART_BEAT); // 创建 heartbeat 配置 xml
    if (xml == NULL)
    {
        LOG_ERROR("心跳 XML 创建失败");
        return false;
    }

    char* encrypt = session_encrypt(xml); // 加密 xml
    if (encrypt == NULL)
    {
        LOG_ERROR("加密心跳 XML 失败");
        return false;
    }
    LOG_VERBOSE("发送加密心跳内容: %s", encrypt);

    const http_resp_t result = post(g_prog_status[thread_idx].auth_cfg.keep_url, encrypt); // 向 keep_url 发送加密数据
    free(encrypt);
    if (result.status != REQUEST_HAVE_RES || result.body_size == 0 || result.body_data == NULL)
    {
        LOG_ERROR("心跳响应失败");
        free(result.body_data);
        return false;
    }

    char* decrypted_data = session_decrypt(result.body_data); // 解密响应内容
    free(result.body_data);
    if (decrypted_data == NULL)
    {
        LOG_ERROR("解密心跳内容失败");
        return false;
    }
    LOG_VERBOSE("心跳响应内容: %s", decrypted_data);

    char* parsed_interval = xml_parser(decrypted_data, "interval"); // 获取心跳内容 (下一次重试时间)
    free(decrypted_data);
    if (parsed_interval == NULL)
    {
        LOG_ERROR("心跳内容解析失败");
        return false;
    }
    g_prog_status[thread_idx].auth_cfg.keep_retry = str_2_uint64(parsed_interval); // 将字符串时间转成 uint64_t 时间
    free(parsed_interval);
    return true;
}

static bool login()
{
    const char* xml = create_xml_payload(LOGIN); // 创建 login 配置 xml
    if (xml == NULL)
    {
        LOG_ERROR("登录 XML 创建失败");
        return false;
    }

    char* encrypt = session_encrypt(xml); // 加密 xml
    if (encrypt == NULL)
    {
        LOG_ERROR("加密登录 XML 失败");
        return false;
    }
    LOG_VERBOSE("发送加密登录内容: %s", encrypt);

    const http_resp_t result = post(g_prog_status[thread_idx].auth_cfg.auth_url, encrypt); // 向 auth_url 发送加密数据
    free(encrypt);
    if (result.status != REQUEST_HAVE_RES || result.body_size == 0 || result.body_data == NULL)
    {
        LOG_ERROR("登录响应失败");
        free(result.body_data);
        return false;
    }
    LOG_VERBOSE("登录响应内容: %s", result.body_data);

    char* decrypted_data = session_decrypt(result.body_data); // 解密响应内容
    free(result.body_data);
    if (decrypted_data == NULL)
    {
        LOG_ERROR("解密登录响应内容失败");
        return false;
    }

    char* parsed_keep_url = xml_parser(decrypted_data, "keep-url"); // 获取 keep_url (包含 CDATA 等字符串)
    if (parsed_keep_url == NULL)
    {
        LOG_ERROR("解析 KeepURL 失败");
        free(decrypted_data);
        return false;
    }

    char* cleaned_keep_url = clean_CDATA(parsed_keep_url); // 获取 keep_url (纯 url, 用于心跳函数)
    free(parsed_keep_url);
    if (cleaned_keep_url == NULL)
    {
        LOG_ERROR("清除 KeepURL CDATA 失败");
        free(decrypted_data);
        return false;
    }
    snprintf(g_prog_status[thread_idx].auth_cfg.keep_url, KEEP_URL_LEN, "%s", safe_str(cleaned_keep_url)); // 将 keep_url 填入认证配置中
    LOG_INFO("Keep-Url: %s", g_prog_status[thread_idx].auth_cfg.keep_url);
    free(cleaned_keep_url);

    char* parsed_term_url = xml_parser(decrypted_data, "term-url"); // 获取 term_url (包含 CDATA 等字符串)
    if (parsed_term_url == NULL)
    {
        LOG_ERROR("解析 TermURL 失败");
        free(decrypted_data);
        return false;
    }

    char* cleaned_term_url = clean_CDATA(parsed_term_url); // 获取 term_url (纯 url, 用于登出函数)
    free(parsed_term_url);
    if (cleaned_term_url == NULL)
    {
        LOG_ERROR("清除 TermURL CDATA 失败");
        free(decrypted_data);
        return false;
    }
    snprintf(g_prog_status[thread_idx].auth_cfg.term_url, TERM_URL_LEN, "%s", safe_str(cleaned_term_url)); // 将 term_url 填入认证配置中
    LOG_INFO("Term-Url: %s", g_prog_status[thread_idx].auth_cfg.term_url);
    free(cleaned_term_url);

    char* parsed_keep_retry = xml_parser(decrypted_data, "keep-retry"); // 获取重试时间长度
    free(decrypted_data);
    if (parsed_keep_retry == NULL)
    {
        LOG_ERROR("解析 KeepRetry 失败");
        return false;
    }
    g_prog_status[thread_idx].auth_cfg.keep_retry = str_2_uint64(parsed_keep_retry); // 将字符串时间转成 uint64_t 时间
    free(parsed_keep_retry);
    LOG_INFO("下一次重试: %" PRIu64 " 秒后", g_prog_status[thread_idx].auth_cfg.keep_retry);
    return true;
}

static bool get_ticket()
{
    LOG_DEBUG("get_ticket 函数入口检查, 使用配置: %" PRIu8 ", 下标: %" PRIu8, g_prog_status[thread_idx].login_cfg.idx, thread_idx);

    const char* xml = create_xml_payload(GET_TICKET); // 创建 get_ticket 用的 xml
    if (xml == NULL)
    {
        LOG_ERROR("创建获取 Ticket XML 失败");
        return false;
    }

    char* encrypt = session_encrypt(xml); // 加密 xml
    if (encrypt == NULL)
    {
        LOG_ERROR("加密获取 Ticket XML 失败");
        return false;
    }
    LOG_VERBOSE("发送加密获取 ticket 内容: %s", encrypt);

    const http_resp_t result = post(g_prog_status[thread_idx].auth_cfg.ticket_url, encrypt); // 向 ticket_url 发送加密内容
    free(encrypt);
    if (result.status != REQUEST_HAVE_RES || result.body_size == 0 || result.body_data == NULL)
    {
        LOG_ERROR("获取 Ticket 响应失败");
        free(result.body_data);
        return false;
    }
    LOG_VERBOSE("获取 Ticket 响应内容: %s", result.body_data);

    char* decrypt = session_decrypt(result.body_data); // 解密响应内容
    free(result.body_data);
    if (decrypt == NULL)
    {
        LOG_ERROR("解密 Ticket 内容失败");
        return false;
    }

    char* parsed_ticket = xml_parser(decrypt, "ticket"); // 获取 ticket
    free(decrypt);
    if (parsed_ticket == NULL)
    {
        LOG_ERROR("解析 Ticket 失败");
        return false;
    }
    snprintf(g_prog_status[thread_idx].auth_cfg.ticket, TICKET_LEN, "%s", safe_str(parsed_ticket)); // 将 ticket 填入认证配置中
    LOG_INFO("Ticket: %s", g_prog_status[thread_idx].auth_cfg.ticket);
    free(parsed_ticket);
    return true;
}

static bool load_cipher(const bytes_t zsm)
{
    LOG_DEBUG("load 函数入口检查, 使用配置: %" PRIu8 ", 下标: %" PRIu8, g_prog_status[thread_idx].login_cfg.idx, thread_idx);

    LOG_DEBUG("接收到的 zsm 数据长度: %zu", zsm.length);
    if (zsm.data == NULL || zsm.length == 0) // 检查 zsm 数据是否为空, 为空则返回 false
    {
        LOG_ERROR("无效的 zsm 数据");
        return false;
    }

    /**
     * 提取 zsm 数据到 str 栈中
     */
    char str[zsm.length + 1];
    memcpy(str, zsm.data, zsm.length);
    str[zsm.length] = '\0';

    const size_t length = strlen(str); // 获取 str 长度
    LOG_DEBUG("原始字符串: %s", str);
    LOG_DEBUG("字符串长度: %zu", length);
    if (length < 4 + 38) // 判断长度, 不足指定长度返回 false
    {
        LOG_ERROR("字符串长度不足");
        return false;
    }

    /**
     * 提取 algo_id
     */
    char algo_id[ALGO_ID_LEN];
    memcpy(algo_id, str + length - 37, ALGO_ID_LEN - 1);
    algo_id[ALGO_ID_LEN - 1] = '\0';
    LOG_INFO("Algo ID: %s", algo_id);

    /**
     * 初始化加解密工厂
     * 如果失败, 返回 false
     */
    if (init_cipher(algo_id) == false)
    {
        LOG_ERROR("初始化加解密工厂失败");
        return false;
    }
    snprintf(g_prog_status[thread_idx].auth_cfg.algo_id, ALGO_ID_LEN, "%s", safe_str(algo_id)); // 将 algo_id 填入认证配置中
    LOG_DEBUG("全局 AlgoID 已更新: %s", g_prog_status[thread_idx].auth_cfg.algo_id);
    return true;
}

static void clean_session()
{
    LOG_DEBUG("清除会话初始化状态");
    destroy_cipher_factory();
    g_prog_status[thread_idx].runtime_status.is_initialized = 0;
}

static bool init_session()
{
    LOG_DEBUG("init_session 函数入口检查, 使用配置: %" PRIu8 ", 下标: %" PRIu8, g_prog_status[thread_idx].login_cfg.idx, thread_idx);

    /**
     * 使用初始 algo_id 向 ticket_url POST 获取数据
     */
    const http_resp_t result = post(g_prog_status[thread_idx].auth_cfg.ticket_url, g_prog_status[thread_idx].auth_cfg.algo_id);
    if (result.status != REQUEST_HAVE_RES || result.body_size == 0 || result.body_data == NULL) // 响应错误或无响应数据, 则返回 false
    {
        LOG_ERROR("初始化会话失败");
        free(result.body_data);
        return false;
    }
    LOG_VERBOSE("会话响应内容: %s", result.body_data);
    const bytes_t zsm = str_2_bytes(result.body_data); // 将响应体数据转成 bytes 类型
    free(result.body_data);

    LOG_DEBUG("开始初始化会话");

    /**
     * 加载加解密工厂
     * 如果失败, 返回 false
     */
    if (load_cipher(zsm) == false)
    {
        LOG_DEBUG("初始化会话失败");
        g_prog_status[thread_idx].runtime_status.is_initialized = 0;
        free(zsm.data);
        return false;
    }
    LOG_DEBUG("初始化会话成功");
    g_prog_status[thread_idx].runtime_status.is_initialized = 1;
    free(zsm.data);
    return true;
}

static bool auth()
{
    LOG_DEBUG("auth 函数入口检查, 使用配置: %" PRIu8 ", 下标: %" PRId8, g_prog_status[thread_idx].login_cfg.idx, thread_idx);

    const char portal_start_tag[] = "<!--//config.campus.js.chinatelecom.com";
    const char portal_end_tag[] = "//config.campus.js.chinatelecom.com-->";

    const http_resp_t resp = get(g_prog_status[thread_idx].last_location); // curl GET last_location 获取认证配置
    if (resp.status != REQUEST_HAVE_RES || resp.body_size == 0 || resp.body_data == NULL) // 如果响应体没有内容 (非 200 响应码), 则返回
    {
        LOG_ERROR("响应体为空, 无法提取认证配置");
        return false;
    }

    char* portal_config = extract_between_tags(resp.body_data, portal_start_tag, portal_end_tag); // 从响应体内容中提取指定内容
    free(resp.body_data);
    if (portal_config == NULL)
    {
        LOG_ERROR("提取门户配置失败");
        return false;
    }

    char* auth_url = xml_parser(portal_config, "auth-url"); // 提取 auth_url (包含 CDATA 等字符)
    if (auth_url == NULL)
    {
        LOG_ERROR("提取 Auth URL 失败");
        return false;
    }

    char* cleaned_auth_url = clean_CDATA(auth_url); // 提取 auth_url (纯 url, 用于登录函数)
    free(auth_url);
    if (cleaned_auth_url == NULL)
    {
        LOG_ERROR("清除 Auth URL 失败");
        return false;
    }
    snprintf(g_prog_status[thread_idx].auth_cfg.auth_url, AUTH_URL_LEN, "%s", safe_str(cleaned_auth_url)); // 将 auth_url 填入认证配置变量中
    LOG_INFO("Auth URL: %s", g_prog_status[thread_idx].auth_cfg.auth_url);
    free(cleaned_auth_url);

    char* ticket_url = xml_parser(portal_config, "ticket-url"); // 提取 ticket_url (包含 CDATA 等字符)
    free(portal_config);
    if (ticket_url == NULL)
    {
        LOG_ERROR("提取 Ticket URL 失败");
        return false;
    }

    char* cleaned_ticket_url = clean_CDATA(ticket_url); // 提取 ticket_url (纯 url, 用于获取 ticket)
    free(ticket_url);
    if (cleaned_ticket_url == NULL)
    {
        LOG_ERROR("清除 Ticket URL CDATA 失败");
        return false;
    }
    snprintf(g_prog_status[thread_idx].auth_cfg.ticket_url, TICKET_URL_LEN, "%s", safe_str(cleaned_ticket_url)); // 将 ticket_url 填入认证配置变量中
    LOG_INFO("Ticket URL: %s", g_prog_status[thread_idx].auth_cfg.ticket_url);

    char* client_ip = extract_url_param(cleaned_ticket_url, "wlanuserip"); // 提取 client_ip
    if (client_ip == NULL)
    {
        LOG_ERROR("提取 Client IP 失败");
        return false;
    }
    snprintf(g_prog_status[thread_idx].auth_cfg.client_ip, IP_LEN, "%s", safe_str(client_ip)); // 将 client_ip 填入认证配置变量中
    LOG_INFO("Client IP: %s", g_prog_status[thread_idx].auth_cfg.client_ip);
    free(client_ip);

    char* ac_ip = extract_url_param(cleaned_ticket_url, "wlanacip"); // 提取 ac_ip
    free(cleaned_ticket_url);
    if (ac_ip == NULL)
    {
        LOG_ERROR("提取 AC IP 失败");
        return false;
    }
    snprintf(g_prog_status[thread_idx].auth_cfg.ac_ip, IP_LEN, "%s", safe_str(ac_ip)); // 将 ac_ip 填入认证配置变量中
    LOG_INFO("AC IP: %s", g_prog_status[thread_idx].auth_cfg.ac_ip);
    free(ac_ip);

    /**
     * 初始化会话
     * 如果失败, 清除会话并返回 false
     */
    if (init_session() == false)
    {
        LOG_FATAL("初始化会话失败");
        clean_session();
        return false;
    }
    LOG_DEBUG("初始化会话完成");

    /**
     * 获取 ticket
     * 如果失败, 清除会话并返回 false
     */
    if (get_ticket() == false)
    {
        LOG_FATAL("获取 Ticket 失败");
        clean_session();
        return false;
    }
    LOG_DEBUG("完成获取 Ticket");

    /**
     * 登录认证
     * 如果失败, 清除会话并返回 false
     */
    if (login() == false)
    {
        LOG_FATAL("登录失败");
        clean_session();
        return false;
    }
    LOG_DEBUG("完成登录");

    g_prog_status[thread_idx].auth_cfg.tick = get_cur_tm_ms(); // 获取当前 tick
    g_prog_status[thread_idx].auth_cfg.auth_time = get_cur_tm_ms(); // 获取认证时间
    LOG_DEBUG("登录时间戳: %" PRIu64, g_prog_status[thread_idx].auth_cfg.auth_time);

    g_prog_status[thread_idx].runtime_status.is_authed = true;
    LOG_INFO("已认证登录");
    return true;
}

static void reset()
{
    if (g_prog_status[thread_idx].runtime_status.is_initialized) // 如果已经初始化会话, 则进入
    {
        if (g_prog_status[thread_idx].runtime_status.is_authed) // 如果已经认证, 则进入
        {
            LOG_DEBUG("配置 %" PRIu8 " 登出, 下标: %" PRId8, g_prog_status[thread_idx].login_cfg.idx, thread_idx);
            term(); // 登出
        }
        clean_session(); // 清理会话
    }
    memset(&g_prog_status[thread_idx].auth_cfg, 0, sizeof(auth_cfg_t)); // 清除 auth_cfg 的内容, 并置零
    memset(&g_prog_status[thread_idx].runtime_status, 0, sizeof(runtime_status_t)); // 清除 runtime_status 的内容, 并置零
    refresh_states();
}

static void clean()
{
    if (g_prog_status[thread_idx].runtime_status.is_initialized) // 如果已经初始化会话, 则进入
    {
        if (g_prog_status[thread_idx].runtime_status.is_authed) // 如果已经认证, 则进入
        {
            LOG_DEBUG("配置 %" PRIu8 " 登出, 下标: %" PRId8, g_prog_status[thread_idx].login_cfg.idx, thread_idx);
            term(); // 登出
        }
        clean_session(); // 清理会话
    }
    memset(&g_prog_status[thread_idx].auth_cfg, 0, sizeof(auth_cfg_t)); // 清除 auth_cfg 的内容, 并置零
    memset(&g_prog_status[thread_idx].runtime_status, 0, sizeof(runtime_status_t)); // 清除 runtime_status 的内容, 并置零
}

static bool run()
{
    static uint8_t retry_timeout = 1;

    switch (check_network_status()) // 检测网络状态
    {
    case REQUEST_SUCCESS: // 返回响应成功 (204 响应码)
        /**
         * 检测是否初始化会话和认证登录
         * 如果已经初始化会话和认证登录, 则进入, 否则按已连接互联网处理
         */
        if (g_prog_status[thread_idx].runtime_status.is_initialized && g_prog_status[thread_idx].runtime_status.is_authed)
        {
            if (g_prog_status[thread_idx].auth_cfg.keep_retry != 0) // 检测重试时间是否为零
            {
                /**
                 * 检测经过的时间是否达到重试时间
                 * 达到就发送心跳包
                 */
                if (get_cur_tm_ms() - g_prog_status[thread_idx].auth_cfg.tick >= g_prog_status[thread_idx].auth_cfg.keep_retry * 1000)
                {
                    LOG_INFO("发送心跳包");
                    uint8_t retry = 1;
                    while (heartbeat() == false)
                    {
                        if (retry > 5)
                        {
                            LOG_FATAL("超过最多重试次数");
                            return false;
                        }
                        LOG_ERROR("配置 %" PRIu8 " 心跳包发送失败, 下标 %" PRIu8 ", 重试: 第 %" PRIu8 " 次, 最多 5 次", g_prog_status[thread_idx].login_cfg.idx, thread_idx, retry);
                        retry++;
                        sleep_ms(1000);
                    }
                    LOG_INFO("下一次重试: %" PRIu64 " 秒后", g_prog_status[thread_idx].auth_cfg.keep_retry);
                    g_prog_status[thread_idx].auth_cfg.tick = get_cur_tm_ms(); // 重新给 tick 赋值
                }
            }
        }
        else
        {
            LOG_INFO("已连接到互联网");
        }
        retry_timeout = 1;
        sleep_ms(1000);
        return true;
    case REQUEST_REDIRECT: // 返回重定向 (302 响应码)
        LOG_INFO("需要认证");
        if (g_prog_status[thread_idx].runtime_status.is_initialized) // 进入认证流程的时候如果会话已经初始化, 重置认证配置参数
        {
            reset();
        }
        if (auth() == false) // 进入认证函数, 可以重试 5 次
        {
            LOG_ERROR("配置 %" PRIu8 " 认证失败, 下标 %" PRIu8, g_prog_status[thread_idx].login_cfg.idx, thread_idx);
            return false;
        }
        retry_timeout = 1;
        sleep_ms(1000);
        return true;
    case REQUEST_WARN: // 返回警告, 会重试 (错误码 28, 响应超时)
        if (retry_timeout > 5)
        {
            LOG_FATAL("超过最多重试次数");
            return false;
        }
        LOG_WARN("网络响应超时, 等待 10 秒后重试, 重试: 第 %" PRIu8 " 次, 最多 5 次", retry_timeout);
        retry_timeout++;
        sleep_ms(10000);
        return true;
    default:
        LOG_ERROR("其它错误");
        retry_timeout = 1;
        sleep_ms(5000);
        return false;
    }
}

int dialer_app(void* arg)
{
    thread_idx = (int8_t)(intptr_t)arg; // 领取线程下标参数
    g_prog_status[thread_idx].runtime_status.is_running = true;
    g_prog_status[thread_idx].thread_id = sim_thread_cur_id(); // 获取当前线程 TID
    LOG_DEBUG("认证线程 %" PRId8 " 创建成功, ID: %" PRIu64 ", 使用配置: %" PRIu8, thread_idx, g_prog_status[thread_idx].thread_id, g_prog_status[thread_idx].login_cfg.idx);

    refresh_states(); // 刷新数据 (algo_id, host_name, client_id, mac_addr)
    get_last_location(); // 获取最后一个 location, 用于获取认证配置

    /**
     * 运行循环
     * is_running 为真且 is_need_reset 为假时保持循环
     * 正在运行且不需要重置时保持循环
     * 如果不运行, 或者需要重置时退出循环
     */
    while (g_prog_status[thread_idx].runtime_status.is_running)
    {
        if (run() == false || g_prog_status[thread_idx].runtime_status.is_need_reset) // 如果 run 函数返回 false 或需要重置, 则退出循环
        {
            g_prog_status[thread_idx].runtime_status.is_running = false;
            break;
        }
    }

    /**
     * 线程退出时的操作
     */
    clean(); // 清除参数
    return 0;
}
