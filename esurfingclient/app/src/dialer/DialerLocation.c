#include "DialerInternal.h"

#include "net/NetClient.h"
#include "net/NetClientInternal.h"

#include "core/States.h"

#include "utils/text/TextUtils.h"
#include "utils/time/TimeUtils.h"
#include "utils/log/Logger.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define MAX_REDIRECT_HOPS 8

/** @brief 大小写不敏感子串查找 */
static const char* find_substr_ci(const char* haystack, const char* needle)
{
    if (haystack == NULL || needle == NULL || *needle == '\0') return NULL;

    const size_t nlen = strlen(needle);
    for (const char* p = haystack; *p != '\0'; p++)
    {
        size_t i = 0;
        while (i < nlen && p[i] != '\0' && tolower((unsigned char)p[i]) == tolower((unsigned char)needle[i]))
        {
            i++;
        }
        if (i == nlen) return p;
    }
    return NULL;
}

/** @brief 尝试提取 HTML `<meta http-equiv="refresh">` 或 JS 跳转目标 */
static bool extract_html_redirect(const char* html, char* out, const size_t out_len)
{
    if (html == NULL || out == NULL || out_len == 0) return false;
    out[0] = '\0';

    /* 已经包含门户 EConfig 配置块时不再做重定向提取 */
    if (strstr(html, "config.campus.js.chinatelecom.com") != NULL) return false;

    /* 1. 查找 <meta ... http-equiv="refresh" ... content="0;url=..."> */
    const char* meta = html;
    while ((meta = find_substr_ci(meta, "<meta")) != NULL)
    {
        const char* tag_end = strchr(meta, '>');
        if (tag_end == NULL) break;

        const size_t tag_len = (size_t)(tag_end - meta + 1);
        if (tag_len < 1024)
        {
            char tag_buf[1024];
            memcpy(tag_buf, meta, tag_len);
            tag_buf[tag_len] = '\0';

            if (find_substr_ci(tag_buf, "refresh") != NULL)
            {
                const char* url_kw = find_substr_ci(tag_buf, "url=");
                if (url_kw != NULL)
                {
                    url_kw += 4;
                    while (*url_kw != '\0' && (*url_kw == ' ' || *url_kw == '\t' || *url_kw == '\'' || *url_kw == '"'))
                    {
                        url_kw++;
                    }
                    const size_t len = strcspn(url_kw, "'\"\r\n> ");
                    if (len > 0 && len < out_len)
                    {
                        memcpy(out, url_kw, len);
                        out[len] = '\0';
                        return true;
                    }
                }
            }
        }
        meta = tag_end + 1;
    }

    /* 2. 查找常见 JS 跳转模式 */
    static const char* const js_patterns[] = {
        "window.location.href",
        "top.location.href",
        "self.location.href",
        "window.location",
        "top.location",
        "self.location",
        "location.href",
        "location.replace",
        "location.assign",
    };

    for (size_t i = 0; i < sizeof(js_patterns) / sizeof(js_patterns[0]); i++)
    {
        const char* p = find_substr_ci(html, js_patterns[i]);
        if (p == NULL) continue;
        p += strlen(js_patterns[i]);
        while (*p != '\0' && (*p == ' ' || *p == '\t' || *p == '=' || *p == '(')) p++;
        if (*p != '\'' && *p != '"') continue;

        const char quote = *p++;
        const char* q_end = strchr(p, quote);
        if (q_end == NULL) continue;

        const size_t len = (size_t)(q_end - p);
        if (len > 0 && len < out_len)
        {
            memcpy(out, p, len);
            out[len] = '\0';
            return true;
        }
    }

    return false;
}

static bool is_redirect_code(const long http_code)
{
    return http_code == HTTP_MOVED_PERMANENTLY || http_code == HTTP_FOUND ||
           http_code == 303 || http_code == 307 || http_code == 308;
}

/** @brief 跟随 HTTP 3xx 与 HTML Meta/JS 跳转直到拿到非重定向页 (带跳数上限防死循环) */
static bool follow_location_redirects(void)
{
    curl_resp_t resp = {0};

    for (int hop = 0; hop < MAX_REDIRECT_HOPS; hop++)
    {
        if (g_need_exit || g_stop_requested)
        {
            curl_resp_free(&resp);
            return false;
        }

        char current_url[sizeof(g_prog_status[0].last_location)];
        snprintf(current_url, sizeof(current_url), "%s", g_prog_status[tl_thread_idx].last_location);
        if (current_url[0] == '\0')
        {
            return false;
        }

        curl_resp_free(&resp);
        resp = get(current_url);

        if (is_redirect_code(resp.http_code))
        {
            if (strcmp(g_prog_status[tl_thread_idx].last_location, current_url) == 0)
            {
                LOG_WARN("重定向地址未变化 (%s), 终止跳转循环", current_url);
                curl_resp_free(&resp);
                return false;
            }
            continue;
        }

        if (resp.http_code == HTTP_OK)
        {
            char next_ref[LOCATION_LEN];
            if (resp.body_data != NULL && extract_html_redirect(resp.body_data, next_ref, sizeof(next_ref)))
            {
                char resolved[sizeof(g_prog_status[0].last_location)];
                net_resolve_url(resolved, sizeof(resolved), current_url, next_ref);
                if (resolved[0] != '\0' && strcmp(resolved, current_url) != 0)
                {
                    snprintf(g_prog_status[tl_thread_idx].last_location,
                             sizeof(g_prog_status[0].last_location), "%s", resolved);
                    LOG_DEBUG("提取到 HTML/JS 跳转地址: %s", g_prog_status[tl_thread_idx].last_location);
                    continue;
                }
            }

            curl_resp_free(&resp);
            return true;
        }

        LOG_WARN("跟随重定向后未获得认证页面, 状态码: %ld", resp.http_code);
        curl_resp_free(&resp);
        return false;
    }

    LOG_WARN("重定向超过最大跳数 (%d 跳), 中止", MAX_REDIRECT_HOPS);
    curl_resp_free(&resp);
    return false;
}

/** @brief 主动向网关发一次明文 HTTP 探测并处理 HTML/JS 跳转, 用于初始或刷新 last_location */
static bool kick_gateway_for_location(void)
{
    static const char* const kick_urls[] = {
        "http://connect.rom.miui.com/generate_204",
        "http://connectivitycheck.platform.hicloud.com/generate_204",
    };

    for (size_t i = 0; i < sizeof(kick_urls) / sizeof(kick_urls[0]); i++)
    {
        if (g_need_exit || g_stop_requested) return false;

        g_prog_status[tl_thread_idx].last_location[0] = '\0';
        curl_resp_t probe = get(kick_urls[i]);
        if (g_prog_status[tl_thread_idx].last_location[0] == '\0' &&
            probe.http_code == HTTP_OK && probe.body_data != NULL)
        {
            char next_ref[LOCATION_LEN];
            if (extract_html_redirect(probe.body_data, next_ref, sizeof(next_ref)))
            {
                char resolved[LAST_LOCATION_LEN];
                net_resolve_url(resolved, sizeof(resolved), kick_urls[i], next_ref);
                snprintf(g_prog_status[tl_thread_idx].last_location, LAST_LOCATION_LEN, "%s", resolved);
            }
        }
        curl_resp_free(&probe);

        if (g_prog_status[tl_thread_idx].last_location[0] != '\0')
        {
            return true;
        }
    }
    return false;
}

static void get_school_ip_symbol(void)
{
    if (g_school_network_symbol[0] != '\0')
    {
        return;
    }
    if (tl_thread_idx < 0)
    {
        return;
    }

    /*
     * 必须用当前线程的 last_location
     * 配置 1 已联网时 g_prog_status[0].last_location 为空
     */
    char* school_ip = extract_url_param(g_prog_status[tl_thread_idx].last_location, "wlanuserip");
    if (school_ip == NULL)
    {
        LOG_ERROR("无法从 last_location 提取 wlanuserip");
        return;
    }

    const char* first_dot = strchr(school_ip, '.');
    const char* second_dot = first_dot ? strchr(first_dot + 1, '.') : NULL;
    if (second_dot == NULL)
    {
        LOG_ERROR("wlanuserip 格式无效: %s", school_ip);
        free(school_ip);
        return;
    }

    const size_t len = (size_t)(second_dot - school_ip);
    if (len == 0 || len >= SCHOOL_NETWORK_SYMBOL)
    {
        LOG_ERROR("校园网标志长度无效: %zu", len);
        free(school_ip);
        return;
    }

    memcpy(g_school_network_symbol, school_ip, len);
    g_school_network_symbol[len] = '\0';
    LOG_DEBUG("获取到校园网标志: %s", g_school_network_symbol);
    free(school_ip);
}

bool refresh_last_location(void)
{
    if (tl_thread_idx < 0) return false;

    char backup[sizeof(g_prog_status[0].last_location)];
    snprintf(backup, sizeof(backup), "%s", g_prog_status[tl_thread_idx].last_location);

    g_prog_status[tl_thread_idx].last_location_lock = false;
    if (kick_gateway_for_location() && follow_location_redirects())
    {
        g_prog_status[tl_thread_idx].last_location_lock = true;
        get_school_ip_symbol();
        return true;
    }

    /* 刷新失败(例如当前短时直通 204): 若有旧门户地址则保留兜底 */
    if (backup[0] != '\0')
    {
        snprintf(g_prog_status[tl_thread_idx].last_location,
                 sizeof(g_prog_status[0].last_location), "%s", backup);
    }
    return false;
}

bool get_last_location(void)
{
    /**
     * last_location 为空: 先主动"踢"一次网关
     *
     * 未认证时访问任意 http 外网地址, 网关会回 302 (或 200 HTML/JS 跳转),
     * 从而获取认证门户入口地址。
     */
    if (g_prog_status[tl_thread_idx].last_location[0] == '\0')
    {
        if (kick_gateway_for_location() == false)
        {
            static _Thread_local uint64_t s_last_warn_ms = 0;

            const uint64_t now = get_steady_tm_ms();
            if (now - s_last_warn_ms >= 60000)
            {
                s_last_warn_ms = now;
                LOG_WARN("还没拿到认证页面地址 (last_location 为空): 请先用浏览器打开任意 http 网站触发跳转");
            }
            return false;
        }
    }

    uint8_t retry = 1;
    bool quit = false;

    while (quit == false)
    {
        if (g_need_exit || g_stop_requested)
        {
            return false;
        }
        const network_status_t net_state = check_network_status(false); // 检查网络状态

        // 状态真的变了才提示 (已联网 / 需要认证), 免得每 10 秒刷一条
        log_network_state_change(net_state);

        switch (net_state)
        {
        case STATUS_OK:
            // 正常连接到互联网
            retry = 1;
            sleep_ms(10000, true);
            break;
        case STATUS_NEED_AUTH:
            // 需要认证
            quit = true;
            break;
        default:
            // 网络错误
            if (retry > 5)
            {
                LOG_ERROR("超过最多重试次数");
                return false;
            }
            LOG_WARN("网络错误, 重试: 第 %" PRIu8 " 次, 最多 5 次", retry);
            retry++;
            sleep_ms(1000, true);
        }
    }

    if (follow_location_redirects() == false)
    {
        return false;
    }

    g_prog_status[tl_thread_idx].last_location_lock = true;
    LOG_DEBUG("配置 %" PRIu8 " 获取认证配置 URL: %s", g_prog_status[tl_thread_idx].cfg.idx, g_prog_status[tl_thread_idx].last_location);

    get_school_ip_symbol(); // 获取校园网特征
    return true;
}
