#include "DialerInternal.h"

#include "net/NetClient.h"

#include "core/States.h"

#include "utils/text/TextUtils.h"
#include "utils/time/TimeUtils.h"
#include "utils/log/Logger.h"

#include <string.h>

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

bool get_last_location(void)
{
    /**
     * last_location 为空: 先主动"踢"一次网关
     *
     * 未认证时访问任意 http 外网地址, 网关会回 302, Location 里就是认证页面地址
     * (curl 的响应头回调会把它绝对化后存进 last_location)。
     *
     * 注意这一步不能省: 联网检测用的是 curl_multi 探测, 只取状态码、不收响应头,
     * 所以 last_location 只能在这里学到。已经联网时这里拿到的是 204,
     * last_location 仍为空 —— 那就等下一轮 (此时也确实不需要认证)。
     */
    if (g_prog_status[tl_thread_idx].last_location[0] == '\0')
    {
        curl_resp_t probe = get("http://connect.rom.miui.com/generate_204");
        curl_resp_free(&probe);

        if (g_prog_status[tl_thread_idx].last_location[0] == '\0')
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

    curl_resp_t resp = {0};

    resp = get(g_prog_status[tl_thread_idx].last_location);

    while (resp.http_code == HTTP_FOUND)
    {
        if (resp.body_data)
        {
            curl_resp_free(&resp);
        }
        resp = get(g_prog_status[tl_thread_idx].last_location);
    }

    if (resp.body_data)
    {
        curl_resp_free(&resp);
    }

    if (resp.http_code != HTTP_OK)
    {
        LOG_WARN("跟随重定向后未获得认证页面, 状态码: %d", resp.http_code);
        return false;
    }

    g_prog_status[tl_thread_idx].last_location_lock = true;
    LOG_DEBUG("配置 %" PRIu8 " 获取认证配置 URL: %s", g_prog_status[tl_thread_idx].login_cfg.idx, g_prog_status[tl_thread_idx].last_location);

    get_school_ip_symbol(); // 获取校园网特征
    return true;
}
