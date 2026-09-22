#include "clients/dialer/dialer_internal.h"

#include "clients/net/NetClient.h"

#include "states/States.h"

#include "utils/PlatformUtils.h"
#include "utils/Logger.h"

#include <string.h>

static void get_school_ip_symbol()
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
    LOG_INFO("获取到校园网标志: %s", g_school_network_symbol);
    free(school_ip);
}

bool get_last_location()
{
    uint8_t retry = 1;
    bool quit = false;

    while (quit == false)
    {
        if (g_need_exit || g_stop_requested)
        {
            return false;
        }
        switch (check_network_status(false)) // 检查网络状态
        {
        case STATUS_OK:
            // 正常连接到互联网
            retry = 1;
            LOG_INFO("已连接至互联网");
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
                LOG_FATAL("超过最多重试次数");
                return false;
            }
            LOG_WARN("网络错误, 重试: 第 %" PRIu8 " 次, 最多 5 次", retry);
            retry++;
            sleep_ms(1000, true);
        }
    }

    curl_resp_t resp = {0};

    resp = get(g_prog_status[tl_thread_idx].last_location, false);

    while (resp.http_code == HTTP_FOUND)
    {
        if (resp.body_data)
        {
            free(resp.body_data);
            resp.body_data = NULL;
            resp.body_size = 0;
        }
        resp = get(g_prog_status[tl_thread_idx].last_location, false);
    }

    if (resp.body_data)
    {
        free(resp.body_data);
        resp.body_data = NULL;
        resp.body_size = 0;
    }

    if (resp.http_code != HTTP_OK)
    {
        LOG_ERROR("跟随重定向后未获得认证页面, 状态码: %d", resp.http_code);
        return false;
    }

    g_prog_status[tl_thread_idx].last_location_lock = true;
    LOG_DEBUG("配置 %" PRIu8 " 获取认证配置 URL: %s", g_prog_status[tl_thread_idx].login_cfg.idx, g_prog_status[tl_thread_idx].last_location);

    get_school_ip_symbol(); // 获取校园网特征
    return true;
}
