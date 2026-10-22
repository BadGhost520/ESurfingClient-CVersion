#include "DialerInternal.h"

#include "net/NetClient.h"

#include "cipher/CipherInterface.h"

#include "core/States.h"

#include "dialer/ErrorCode.h"


#include "utils/text/TextUtils.h"
#include "utils/time/TimeUtils.h"
#include "utils/time/TimeControl.h"
#include "dialer/LogoutState.h"
#include "utils/runtime/Watchdog.h"
#include "utils/log/Logger.h"

#include <stdio.h>

/**
 * @brief 构造一个报文并加密 (调用方负责 free)
 * @param kind 报文类型
 * @return 加密后的文本, 失败返回 NULL
 */
static char* dialer_encrypt_payload(const XmlChoose kind)
{
    char* xml = create_xml_payload(kind);
    if (xml == NULL)
    {
        LOG_ERROR("报文构造失败 (类型 %d)", (int)kind);
        return NULL;
    }

    char* encrypt = session_encrypt(xml);
    free(xml);
    if (encrypt == NULL)
    {
        LOG_ERROR("报文加密失败 (类型 %d)", (int)kind);
        return NULL;
    }

    LOG_VERBOSE("发送加密内容 (类型 %d): %s", (int)kind, encrypt);
    return encrypt;
}

/**
 * @brief 走一遍"构造报文 → 加密 → POST → 解密"
 *
 * 认证流程里心跳 / 登录 / 取 ticket 都是这五步, 只是地址和日志措辞不同;
 * 响应失败时顺带把服务端的错误码记下来, 交给认证失败分类去决定怎么等
 *
 * @param kind 报文类型
 * @param url 目标地址
 * @param what 日志里的动作名 (如 "心跳")
 * @return 解密后的明文 (调用方负责 free), 失败返回 NULL
 */
char* dialer_request_plain(const XmlChoose kind, const char* url, const char* what)
{
    char* encrypt = dialer_encrypt_payload(kind);
    if (encrypt == NULL) return NULL;

    curl_resp_t resp = post(url, encrypt);
    free(encrypt);

    if (resp.http_code != HTTP_OK || resp.body_size == 0 || resp.body_data == NULL)
    {
        LOG_ERROR("%s响应失败", safe_str(what));
        error_code_record(&resp);
        curl_resp_free(&resp);
        return NULL;
    }
    LOG_VERBOSE("%s响应内容: %s", safe_str(what), resp.body_data);

    char* plain = session_decrypt(resp.body_data);
    curl_resp_free(&resp);
    if (plain == NULL)
    {
        LOG_ERROR("%s响应解密失败", safe_str(what));
        return NULL;
    }
    return plain;
}

bool term(void)
{
    char* encrypt = dialer_encrypt_payload(TERM);
    if (encrypt == NULL) return false;

    curl_resp_t resp = post(g_prog_status[tl_thread_idx].auth_cfg.term_url, encrypt); // 向 term_url 发送加密数据
    uint8_t retry = 1;
    while (resp.status != STATUS_OK && resp.status != STATUS_NEED_AUTH && resp.http_code != HTTP_OK)
    {
        if (retry > 5)
        {
            LOG_FATAL("超过最多重试次数, 返回");
            free(encrypt);
            curl_resp_free(&resp);
            return false;
        }
        LOG_ERROR("配置 %" PRIu8 " 登出失败, 下标 %" PRIu8 ", 错误代码: %d, 重试: 第 %" PRIu8 " 次, 最多 5 次", g_prog_status[tl_thread_idx].login_cfg.idx, tl_thread_idx, resp.status, retry);
        retry++;
        sleep_ms(1000, true);
        curl_resp_free(&resp); // 丢掉上一次的响应体, 否则重试会把它漏掉
        resp = post(g_prog_status[tl_thread_idx].auth_cfg.term_url, encrypt); // 向 term_url 发送加密数据 (重试)
    }
    free(encrypt);
    curl_resp_free(&resp);

    g_prog_status[tl_thread_idx].auth_cfg.auth_time = 0;
    g_prog_status[tl_thread_idx].runtime_status.is_authed = false;

    // 已经登出了, 存档没用了 (留着的话下次启动会白发一次无用的请求)
    logout_state_clear(g_prog_status[tl_thread_idx].login_cfg.idx);
    return true;
}

bool heartbeat(void)
{
    char* decrypted_data = dialer_request_plain(HEART_BEAT, g_prog_status[tl_thread_idx].auth_cfg.keep_url, "心跳");
    if (decrypted_data == NULL) return false;
    LOG_VERBOSE("心跳响应内容: %s", decrypted_data);

    char* parsed_interval = xml_parser(decrypted_data, "interval"); // 获取心跳内容 (下一次重试时间)
    free(decrypted_data);
    if (parsed_interval == NULL)
    {
        LOG_ERROR("心跳内容解析失败");
        return false;
    }

    const uint64_t tmp_retry = str2uint64(parsed_interval); // 将字符串时间转成 uint64_t 时间
    if (tmp_retry < (unsigned long)g_op_timeout * 5)
    {
        g_prog_status[tl_thread_idx].auth_cfg.keep_retry = tmp_retry;
    }
    else
    {
        g_prog_status[tl_thread_idx].auth_cfg.keep_retry = tmp_retry - g_op_timeout * 5;
    }

    free(parsed_interval);
    return true;
}

bool login(void)
{
    char* decrypted_data = dialer_request_plain(LOGIN, g_prog_status[tl_thread_idx].auth_cfg.auth_url, "登录");
    if (decrypted_data == NULL) return false;

    char* parsed_keep_url = xml_parser(decrypted_data, "keep-url"); // 获取 keep_url (包含 CDATA 等字符串)
    if (parsed_keep_url == NULL)
    {
        LOG_ERROR("解析 KeepURL 失败");
        free(decrypted_data);
        return false;
    }

    char* cleaned_keep_url = clean_cdata(parsed_keep_url); // 获取 keep_url (纯 url, 用于心跳函数)
    free(parsed_keep_url);
    if (cleaned_keep_url == NULL)
    {
        LOG_ERROR("清除 KeepURL CDATA 失败");
        free(decrypted_data);
        return false;
    }
    snprintf(g_prog_status[tl_thread_idx].auth_cfg.keep_url, KEEP_URL_LEN, "%s", safe_str(cleaned_keep_url)); // 将 keep_url 填入认证配置中
    LOG_INFO("Keep-Url: %s", g_prog_status[tl_thread_idx].auth_cfg.keep_url);
    free(cleaned_keep_url);

    char* parsed_term_url = xml_parser(decrypted_data, "term-url"); // 获取 term_url (包含 CDATA 等字符串)
    if (parsed_term_url == NULL)
    {
        LOG_ERROR("解析 TermURL 失败");
        free(decrypted_data);
        return false;
    }

    char* cleaned_term_url = clean_cdata(parsed_term_url); // 获取 term_url (纯 url, 用于登出函数)
    free(parsed_term_url);
    if (cleaned_term_url == NULL)
    {
        LOG_ERROR("清除 TermURL CDATA 失败");
        free(decrypted_data);
        return false;
    }
    snprintf(g_prog_status[tl_thread_idx].auth_cfg.term_url, TERM_URL_LEN, "%s", safe_str(cleaned_term_url)); // 将 term_url 填入认证配置中
    LOG_INFO("Term-Url: %s", g_prog_status[tl_thread_idx].auth_cfg.term_url);
    free(cleaned_term_url);

    char* parsed_keep_retry = xml_parser(decrypted_data, "keep-retry"); // 获取重试时间长度
    free(decrypted_data);
    if (parsed_keep_retry == NULL)
    {
        LOG_ERROR("解析 KeepRetry 失败");
        return false;
    }

    const uint64_t tmp_retry = str2uint64(parsed_keep_retry); // 将字符串时间转成 uint64_t 时间
    if (tmp_retry < (unsigned long)g_op_timeout * 5)
    {
        g_prog_status[tl_thread_idx].auth_cfg.keep_retry = tmp_retry;
    }
    else
    {
        g_prog_status[tl_thread_idx].auth_cfg.keep_retry = tmp_retry - g_op_timeout * 5;
    }

    free(parsed_keep_retry);
    LOG_INFO("下一次重试: %" PRIu64 " 秒后", g_prog_status[tl_thread_idx].auth_cfg.keep_retry);
    return true;
}

/**
 * @brief 等待网络进入需要认证的状态
 *
 * 已联网时按 10 秒一次轮询, 网络错误时按 1 秒一次重试, 最多 5 次
 * @return 等待结果
 */
WaitResult wait_need_auth(void)
{
    uint8_t retry_network = 1;

    while (g_need_exit == false && g_stop_requested == 0)
    {
        /**
         * 打卡: 只作为兜底 (精确的打卡在 sleep_ms 与 get/post 里, 见 dialer_app)
         */
        watchdog_pet_network();

        /**
         * 兜底: 配置未加载完成时交回主循环 ("保存并应用"已改为重启,
         * 正常不会在这里把 g_cfg_loaded 置假)
         */
        if (g_cfg_loaded == false) return WAIT_CFG_CHANGED;

        if (supervisor_gone())
        {
            LOG_WARN("守护进程已退出, 本进程一并退出");
            return WAIT_EXIT;
        }

        /**
         * 认证进程里顺带看住时间窗口:
         * 允许时段可能在等待网络的过程中关闭, 这时要交回主循环去等待,
         * 而不是继续把网络轮询做完
         */
        if (g_prog_role == ROLE_AUTH)
        {
            time_control_sync();
            if (g_prog_status[0].runtime_status.is_time_disabled)
            {
                return WAIT_TIME_CLOSED;
            }
        }

        switch (check_network_status(true)) // 检查网络状态
        {
        case STATUS_OK:
            // 正常连接到互联网
            retry_network = 1;
            LOG_INFO("已连接至互联网");
            sleep_ms(3000, false);
            break;
        case STATUS_NEED_AUTH:
            // 需要认证
            return WAIT_READY;
        default:
            // 网络错误
            if (retry_network > 5)
            {
                LOG_FATAL("超过最多重试次数");
                return WAIT_RETRY_EXHAUSTED;
            }
            LOG_WARN("网络错误, 重试: 第 %" PRIu8 " 次, 最多 5 次", retry_network);
            retry_network++;
            sleep_ms(1000, true);
        }
    }

    return WAIT_EXIT;
}

/**
 * @brief 补做上次没来得及做的登出
 *
 * 进程被强杀 (断电 / 看门狗判定卡死 / 控制端口被占只能硬杀) 时跑不到 clean(),
 * 会话会留在服务端在线状态。上次登录时把登出需要的现场存了档, 这里读回来、
 * 重建加解密工厂、补发一次 term 请求。
 *
 * 尽力而为: 客户端 IP 可能已经变了 (DHCP 重新分配), 那种情况登出会失败 ——
 * 那就交给服务器超时把会话踢掉。无论成败都清掉存档, 否则每次启动都会白发一次
 * 注定失败的请求。
 */
void logout_previous_session(void)
{
    if (logout_state_load(&g_prog_status[0]) == false) return;

    LOG_WARN("检测到 logout 文件, 尝试登出");

    /**
     * term() / init_cipher() 都按 tl_thread_idx 取状态。认证进程就一个账号、
     * 下标是 0, 而走到这里时 tl_thread_idx 还是初值 -1 —— 不设就会写到
     * g_prog_status[-1] 上
     */
    tl_thread_idx = 0;

    const bool inited = init_cipher(g_prog_status[0].auth_cfg.algo_id);

    if (inited == true)
    {
        if (term() == true)
        {
            LOG_INFO("登出完成, 会话已从服务端释放");
        }
        else
        {
            LOG_WARN("登出失败 (客户端 IP 可能已经变了), 交给服务器超时下线");
        }
        destroy_cipher_factory();
    }
    else
    {
        LOG_WARN("恢复加解密工厂失败, 无法补登出");
    }

    logout_state_clear(g_prog_status[0].login_cfg.idx);
}
