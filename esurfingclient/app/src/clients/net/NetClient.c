#include "clients/net/NetClient.h"

#include "clients/net/NetClientInternal.h"

#include "states/States.h"

#include "utils/PlatformUtils.h"
#include "utils/Watchdog.h"
#include "utils/Logger.h"

#ifdef _WIN32
#else
#include <string.h>
#include <stdlib.h>
#endif

#define MAX_LEN 128

#define REQ_CONTENT_TYPE "Content-Type: application/x-www-form-urlencoded"
#define REQ_ACCEPT "Accept: text/html,text/xml,application/xhtml+xml,application/x-javascript,*/*"

_Thread_local char s_request_url[LOCATION_LEN] = {0};

curl_resp_t post(const char* url, const char* data)
{
    LOG_DEBUG("POST 地址: %s", url);
    LOG_DEBUG("POST 数据: %s", data);

    curl_resp_t resp = {0};
    char errbuf[CURL_ERROR_SIZE] = {0};

    char md5_hash_str[MAX_LEN] = {0};
    char ua[MAX_LEN] = {0};
    char c_id[MAX_LEN] = {0};
    char a_id[MAX_LEN] = {0};
    char cdc_sid[MAX_LEN] = {0};
    char cdc_d[MAX_LEN] = {0};
    char cdc_a[MAX_LEN] = {0};
    char* md5_hash = calc_md5(data);
    if (!md5_hash)
    {
        LOG_ERROR("计算 MD5 失败");
        resp.status = STATUS_ERROR;
        return resp;
    }

    snprintf(md5_hash_str, MAX_LEN, "CDC-Checksum: %s", safe_str(md5_hash));
    free(md5_hash);
    snprintf(ua, MAX_LEN, "User-Agent: %s", safe_str(g_prog_status[tl_thread_idx].login_cfg.user_agent));
    snprintf(c_id, MAX_LEN, "Client-ID: %s", safe_str(g_prog_status[tl_thread_idx].auth_cfg.client_id));
    snprintf(a_id, MAX_LEN, "Algo-ID: %s", safe_str(g_prog_status[tl_thread_idx].auth_cfg.algo_id));
    snprintf(cdc_sid, MAX_LEN, "CDC-SchoolId: %s", safe_str(s_school_id));
    snprintf(cdc_d, MAX_LEN, "CDC-Domain: %s", safe_str(s_domain));
    snprintf(cdc_a, MAX_LEN, "CDC-Area: %s", safe_str(s_area));

    LOG_VERBOSE("POST 添加头 %s", md5_hash_str);
    LOG_VERBOSE("POST 添加头 %s", REQ_CONTENT_TYPE);
    LOG_DEBUG("POST 添加头 %s", ua);
    LOG_VERBOSE("POST 添加头 %s", REQ_ACCEPT);
    LOG_VERBOSE("POST 添加头 %s", c_id);
    LOG_DEBUG("POST 添加头 %s", a_id);
    LOG_VERBOSE("POST 添加头 %s", cdc_sid);
    LOG_VERBOSE("POST 添加头 %s", cdc_d);
    LOG_VERBOSE("POST 添加头 %s", cdc_a);
    LOG_VERBOSE("线程下标: %" PRId8, tl_thread_idx);

    struct curl_slist* headers = NULL;

    headers = curl_slist_append(headers, md5_hash_str);
    headers = curl_slist_append(headers, REQ_CONTENT_TYPE);
    headers = curl_slist_append(headers, ua);
    headers = curl_slist_append(headers, REQ_ACCEPT);
    headers = curl_slist_append(headers, c_id);
    headers = curl_slist_append(headers, a_id);
    headers = curl_slist_append(headers, cdc_sid);
    headers = curl_slist_append(headers, cdc_d);
    headers = curl_slist_append(headers, cdc_a);

    CURL* curl = curl_easy_init();
    if (curl == NULL)
    {
        LOG_ERROR("curl 初始化失败");
        resp.status = STATUS_INIT_ERROR;
        curl_slist_free_all(headers);
        return resp;
    }
    LOG_VERBOSE("curl 初始化完成, curl: %p", curl);

    LOG_VERBOSE("设置 curl 选项");
    // POST URL
    curl_easy_setopt(curl, CURLOPT_URL, url);
    // 连接超时时长
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, g_conn_timeout);
    // 总超时时长
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, g_op_timeout);

    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp);

#ifdef __OPENWRT__
    curl_easy_setopt(curl, CURLOPT_OPENSOCKETFUNCTION, open_socket_callback);
#endif

    LOG_VERBOSE("执行 CURL");
    watchdog_pet_network(); // 打卡: 下面这句最长会阻塞"连接超时 + 操作超时"
    const CURLcode curl_code = curl_easy_perform(curl);
    if (curl_code != CURLE_OK)
    {
        // 先输出调试信息（此时 curl 句柄仍然有效）
        log_curl_error(curl, curl_code, errbuf, url, "post");
        // 再清理资源
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        resp.status = curl_err_msg_out(curl_code);
        resp.curl_code = curl_code;
        return resp;
    }

    LOG_VERBOSE("获取响应码");
    long resp_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp_code);

    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);

    if (resp_code == 200)
    {
        LOG_DEBUG("请求成功, 响应码: 200");
        resp.http_code = HTTP_OK;
        resp.status = STATUS_OK;
        return resp;
    }
    if (resp_code == 204)
    {
        LOG_DEBUG("无内容, 响应码: 204");
        resp.http_code = HTTP_NO_CONTENT;
        resp.status = STATUS_OK;
        return resp;
    }
    if (resp_code == 302)
    {
        LOG_DEBUG("重定向, 响应码: 302");
        if (tl_thread_idx > -1) LOG_DEBUG("重定向至: %s", g_prog_status[tl_thread_idx].last_location);
        resp.http_code = HTTP_FOUND;
        resp.status = STATUS_OK;
        return resp;
    }

    LOG_ERROR("意外的 HTTP 响应码: %ld", resp_code);
    resp.http_code = resp_code;
    resp.status = STATUS_ERROR;
    return resp;
}

curl_resp_t get(const char* url, const bool connect_only)
{
    LOG_DEBUG("GET 地址: %s", url);

    curl_resp_t resp = {0};
    char errbuf[CURL_ERROR_SIZE] = {0};

    char ua[MAX_LEN] = {0};
    char c_id[MAX_LEN] = {0};

    struct curl_slist* headers = NULL;
    struct curl_slist *resolve = NULL;

    if (tl_thread_idx > -1)
    {
        snprintf(s_request_url, sizeof(s_request_url), "%s", safe_str(url));
        snprintf(ua, MAX_LEN, "User-Agent: %s", safe_str(g_prog_status[tl_thread_idx].login_cfg.user_agent));
        snprintf(c_id, MAX_LEN, "Client-ID: %s", safe_str(g_prog_status[tl_thread_idx].auth_cfg.client_id));

        LOG_DEBUG("GET 添加头 %s", ua);
        LOG_VERBOSE("GET 添加头 %s", REQ_ACCEPT);
        LOG_VERBOSE("GET 添加头 %s", c_id);
        LOG_VERBOSE("线程下标: %" PRId8, tl_thread_idx);

        headers = curl_slist_append(headers, ua);
        headers = curl_slist_append(headers, REQ_ACCEPT);
        headers = curl_slist_append(headers, c_id);
    }

    CURL* curl = curl_easy_init();
    if (curl == NULL)
    {
        LOG_ERROR("curl 初始化失败");
        resp.status = STATUS_INIT_ERROR;
        curl_slist_free_all(headers);
        return resp;
    }
    LOG_VERBOSE("curl 初始化完成, curl = %p", curl);

    LOG_VERBOSE("设置 curl 选项");
    // GET URL
    curl_easy_setopt(curl, CURLOPT_URL, url);
    // 连接超时时长
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, g_conn_timeout);
    // 总超时时长
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, g_op_timeout);
    // 是否跟随重定向 (当前否)
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);

    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
    if (connect_only == true) // 判断是否为仅连接模式 (检测网络状态用)
    {
        resolve = curl_slist_append(resolve, "connect.rom.miui.com:80:220.181.104.183");
        curl_easy_setopt(curl, CURLOPT_RESOLVE, resolve); // 自定义解析地址

        curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 2L); // 仅连接
    }
    if (tl_thread_idx > -1)
    {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_cb);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp);
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);
    #ifdef __OPENWRT__
        curl_easy_setopt(curl, CURLOPT_OPENSOCKETFUNCTION, open_socket_callback);
    #endif
    }

    LOG_VERBOSE("执行 CURL");
    watchdog_pet_network(); // 打卡: 下面这句最长会阻塞"连接超时 + 操作超时"
    const CURLcode curl_code = curl_easy_perform(curl);
    if (curl_code != CURLE_OK)
    {
        // 先输出调试信息（此时 curl 句柄仍然有效）
        log_curl_error(curl, curl_code, errbuf, url, "get");
        // 再清理资源
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        curl_slist_free_all(resolve);
        resp.status = curl_err_msg_out(curl_code);
        resp.curl_code = curl_code;
        return resp;
    }

    LOG_VERBOSE("获取响应码");
    long resp_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp_code);

    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    curl_slist_free_all(resolve);

    if (resp_code == 200)
    {
        LOG_DEBUG("请求成功, 响应码: 200");
        resp.http_code = HTTP_OK;
        resp.status = STATUS_OK;
        return resp;
    }
    if (resp_code == 204)
    {
        LOG_VERBOSE("无内容, 响应码: 204");
        resp.http_code = HTTP_NO_CONTENT;
        resp.status = STATUS_OK;
        return resp;
    }
    if (resp_code == 301)
    {
        LOG_DEBUG("永久移动, 重定向, 响应码: 301");
        resp.http_code = HTTP_MOVED_PERMANENTLY;
        resp.status = STATUS_OK;
        return resp;
    }
    if (resp_code == 302)
    {
        LOG_DEBUG("临时移动, 重定向, 响应码: 302");
        if (tl_thread_idx > -1) LOG_DEBUG("重定向至: %s", g_prog_status[tl_thread_idx].last_location);
        resp.http_code = HTTP_FOUND;
        resp.status = STATUS_NEED_AUTH;
        return resp;
    }

    LOG_ERROR("意外的 HTTP 响应码: %ld", resp_code);
    resp.http_code = resp_code;
    resp.status = STATUS_ERROR;
    return resp;
}
