#include "DialerInternal.h"

#include "utils/log/Logger.h"

#include <curl/curl.h>
#include <stdio.h>
#include <string.h>

/** 认证服务器只有这两台, 端口都是 7001 */
#define AUTH_SERVER_PORT 7001

/** 单台服务器的测速上限: 连不上就算了, 不能拖住整个认证流程 */
#define AUTH_SERVER_TIMEOUT_MS 2000L

static const char* const s_auth_servers[] = {
    "14.146.227.141",
    "121.8.177.212",
};

#define AUTH_SERVER_COUNT (sizeof(s_auth_servers) / sizeof(s_auth_servers[0]))

/**
 * @brief 并发测两台认证服务器的 TCP 连接延迟, 选快的那台
 *
 * 门户页面里也会给一个 auth-url, 但那只是 302 那一跳顺带带出来的, 不保证是最快的,
 * 也不保证一直可用。既然服务器地址是固定的, 干脆自己测: 并发建 TCP 连接
 * (CONNECT_ONLY, 不发数据), 比连接耗时, 谁快用谁。
 *
 * @param out 输出 "http://ip:7001"
 * @param out_len 缓冲长度
 * @return 是否至少测通一台 (都测不通时退化为第一台, 让后续请求自己暴露错误)
 */
bool select_auth_server(char* out, const size_t out_len)
{
    if (out == NULL || out_len == 0) return false;
    out[0] = '\0';

    CURLM* multi = curl_multi_init();
    if (multi == NULL) return false;

    CURL* handles[AUTH_SERVER_COUNT];
    double connect_time[AUTH_SERVER_COUNT];
    size_t count = 0;

    for (size_t i = 0; i < AUTH_SERVER_COUNT; i++)
    {
        char url[64];
        snprintf(url, sizeof(url), "http://%s:%d", s_auth_servers[i], AUTH_SERVER_PORT);

        CURL* handle = curl_easy_init();
        if (handle == NULL) continue;

        curl_easy_setopt(handle, CURLOPT_URL, url);
        curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT_MS, AUTH_SERVER_TIMEOUT_MS);
        curl_easy_setopt(handle, CURLOPT_TIMEOUT_MS, AUTH_SERVER_TIMEOUT_MS);
        curl_easy_setopt(handle, CURLOPT_CONNECT_ONLY, 1L); // 只建 TCP 连接, 不发请求
        curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(handle, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);

        if (curl_multi_add_handle(multi, handle) != CURLM_OK)
        {
            curl_easy_cleanup(handle);
            continue;
        }

        handles[count] = handle;
        connect_time[count] = -1.0;
        count++;
    }

    if (count == 0)
    {
        curl_multi_cleanup(multi);
        return false;
    }

    int running = 0;
    curl_multi_perform(multi, &running);

    while (running > 0)
    {
        int wait_fds = 0;
        curl_multi_wait(multi, NULL, 0, 200, &wait_fds);
        curl_multi_perform(multi, &running);

        CURLMsg* msg = NULL;
        int left = 0;
        while ((msg = curl_multi_info_read(multi, &left)) != NULL)
        {
            if (msg->msg != CURLMSG_DONE || msg->data.result != CURLE_OK) continue;

            for (size_t i = 0; i < count; i++)
            {
                if (handles[i] != msg->easy_handle) continue;

                double seconds = 0.0;
                if (curl_easy_getinfo(msg->easy_handle, CURLINFO_CONNECT_TIME, &seconds) == CURLE_OK)
                {
                    connect_time[i] = seconds;
                }
            }
        }
    }

    // 取连接耗时最小的那台
    size_t best = count;
    for (size_t i = 0; i < count; i++)
    {
        if (connect_time[i] < 0.0) continue;
        if (best == count || connect_time[i] < connect_time[best]) best = i;
    }

    bool ok = (best < count);
    if (ok)
    {
        snprintf(out, out_len, "http://%s:%d", s_auth_servers[best], AUTH_SERVER_PORT);
    }
    else
    {
        // 都测不通: 退化成第一台, 让后续请求把真正的错误暴露出来
        snprintf(out, out_len, "http://%s:%d", s_auth_servers[0], AUTH_SERVER_PORT);
    }

    /**
     * 只在选择结果变化时打 INFO: 认证失败重试时每轮都会重新选一次,
     * 每次都打就刷屏了
     */
    static _Thread_local char s_last_server[64] = "";

    if (strcmp(s_last_server, out) != 0)
    {
        snprintf(s_last_server, sizeof(s_last_server), "%s", out);

        if (ok)
        {
            LOG_INFO("选择认证服务器: %s:%d (连接耗时 %.0f ms)", s_auth_servers[best], AUTH_SERVER_PORT,
                     connect_time[best] * 1000.0);
        }
        else
        {
            LOG_WARN("两台认证服务器都连不上, 先按 %s:%d 试", s_auth_servers[0], AUTH_SERVER_PORT);
        }
    }
    else
    {
        LOG_DEBUG("认证服务器: %s", out);
    }

    for (size_t i = 0; i < count; i++)
    {
        curl_multi_remove_handle(multi, handles[i]);
        curl_easy_cleanup(handles[i]);
    }
    curl_multi_cleanup(multi);

    return ok;
}
