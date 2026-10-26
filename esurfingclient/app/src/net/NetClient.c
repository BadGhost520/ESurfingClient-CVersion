#include "net/NetClient.h"

#include "NetClientInternal.h"

#include "core/States.h"

#include "utils/text/TextUtils.h"
#include "utils/runtime/Watchdog.h"
#include "utils/log/Logger.h"

#ifdef _WIN32
#else
#include <string.h>
#include <stdlib.h>
#endif

#define MAX_LEN 128

#define REQ_CONTENT_TYPE "Content-Type: application/x-www-form-urlencoded"
#define REQ_ACCEPT "Accept: text/html,text/xml,application/xhtml+xml,application/x-javascript,*/*"

_Thread_local char g_request_url[LOCATION_LEN] = {0};

/**
 * @brief 域名 → 固定 IP 映射 (不经系统 DNS)
 *
 * 校园网的 DNS 本身经常不可用或被劫持, 而这些地址又是固定的, 所以直接写死:
 * - `connect.rom.miui.com`: 踢网关那一下
 * - `connectivitycheck.platform.hicloud.com`: 联网检测(探测线程自己也有表, 这里给 get 兜底)
 * - `enet.10000.gd.cn`: 门户地址, 302 的 Location 里可能出现这个域名
 *
 * 每个域名给两个地址, 第一个连不上时换第二个重试一次。
 */
typedef struct
{
    const char* host;
    const char* ips[2];
} fixed_host_t;

static const fixed_host_t s_fixed_hosts[] = {
    {"connect.rom.miui.com", {"202.69.4.32", "220.181.104.183"}},
    {"connectivitycheck.platform.hicloud.com", {"183.61.185.93", "125.88.255.53"}},
    {"enet.10000.gd.cn", {"125.88.59.131", "61.140.12.23"}},
};

/** @brief 取 URL 里的端口, 没写就是 80 */
static unsigned url_port(const char* url)
{
    if (url == NULL) return 80;

    const char* start = strstr(url, "://");
    start = (start != NULL) ? start + 3 : url;
    start += strcspn(start, ":/");

    if (*start != ':') return 80;

    const unsigned port = (unsigned)strtoul(start + 1, NULL, 10);
    return port == 0 ? 80 : port;
}

/** @brief 找 URL 的 host 在固定 IP 表里的条目 */
static const fixed_host_t* fixed_host_for_url(const char* url)
{
    if (url == NULL) return NULL;

    const char* start = strstr(url, "://");
    start = (start != NULL) ? start + 3 : url;

    const size_t len = strcspn(start, ":/?#");
    if (len == 0 || len >= 128) return NULL;

    for (size_t i = 0; i < sizeof(s_fixed_hosts) / sizeof(s_fixed_hosts[0]); i++)
    {
        if (strlen(s_fixed_hosts[i].host) == len && strncmp(start, s_fixed_hosts[i].host, len) == 0)
        {
            return &s_fixed_hosts[i];
        }
    }
    return NULL;
}

/** @brief 为某个固定 IP 生成 curl 的 resolve 条目 (host:port:ip) */
static struct curl_slist* fixed_host_resolve(const fixed_host_t* host, const char* ip, const char* url)
{
    if (host == NULL || ip == NULL) return NULL;

    char entry[256];
    snprintf(entry, sizeof(entry), "%s:%u:%s", host->host, url_port(url), ip);

    LOG_DEBUG("固定解析: %s", entry);
    return curl_slist_append(NULL, entry);
}

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
    snprintf(ua, MAX_LEN, "User-Agent: %s", safe_str(g_prog_status[tl_thread_idx].cfg.user_agent));
    snprintf(c_id, MAX_LEN, "Client-ID: %s", safe_str(g_prog_status[tl_thread_idx].auth_cfg.client_id));
    snprintf(a_id, MAX_LEN, "Algo-ID: %s", safe_str(g_prog_status[tl_thread_idx].auth_cfg.algo_id));
    snprintf(cdc_sid, MAX_LEN, "CDC-SchoolId: %s", safe_str(g_school_id));
    snprintf(cdc_d, MAX_LEN, "CDC-Domain: %s", safe_str(g_school_domain));
    snprintf(cdc_a, MAX_LEN, "CDC-Area: %s", safe_str(g_area));

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
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_cb); // 错误码在响应头上, POST 也要收
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &resp);
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

void curl_resp_free(curl_resp_t* resp)
{
    if (resp == NULL) return;

    free(resp->body_data);
    resp->body_data = NULL;
    resp->body_size = 0;
}

curl_resp_t get(const char* url)
{
    LOG_DEBUG("GET 地址: %s", url);

    curl_resp_t resp = {0};
    char errbuf[CURL_ERROR_SIZE] = {0};

    char ua[MAX_LEN] = {0};
    char c_id[MAX_LEN] = {0};

    struct curl_slist* headers = NULL;

    if (tl_thread_idx > -1)
    {
        snprintf(g_request_url, sizeof(g_request_url), "%s", safe_str(url));
        snprintf(ua, MAX_LEN, "User-Agent: %s", safe_str(g_prog_status[tl_thread_idx].cfg.user_agent));
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

    /**
     * 固定 IP 解析: 表里有的域名不走系统 DNS, 直接指定要连的地址
     */
    struct curl_slist* resolve = NULL;
    const fixed_host_t* fixed = fixed_host_for_url(url);
    if (fixed != NULL && fixed->ips[0] != NULL)
    {
        resolve = fixed_host_resolve(fixed, fixed->ips[0], url);
        if (resolve != NULL) curl_easy_setopt(curl, CURLOPT_RESOLVE, resolve);
    }

    // 连接超时时长
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, g_conn_timeout);
    // 总超时时长
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, g_op_timeout);
    // 是否跟随重定向 (当前否)
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);

    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);

    if (tl_thread_idx > -1)
    {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_cb);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &resp);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp);
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);
    #ifdef __OPENWRT__
        curl_easy_setopt(curl, CURLOPT_OPENSOCKETFUNCTION, open_socket_callback);
    #endif
    }

    LOG_VERBOSE("执行 CURL");
    watchdog_pet_network(); // 打卡: 下面这句最长会阻塞"连接超时 + 操作超时"
    CURLcode curl_code = curl_easy_perform(curl);

    /**
     * 第一个固定 IP 连不上时换第二个再试一次
     *
     * 只在"压根没连上"这类错误上重试 (连不上/超时), 已经拿到响应的不重试 ——
     * 免得把一个正常的 302/404 当成失败又打一遍。
     */
    if (curl_code != CURLE_OK && fixed != NULL && fixed->ips[1] != NULL)
    {
        LOG_DEBUG("固定 IP %s 请求失败 (CURLcode %d), 换 %s 重试", fixed->ips[0], (int)curl_code, fixed->ips[1]);

        // 第一次可能已经写进一部分响应体, 重试前清掉
        free(resp.body_data);
        resp.body_data = NULL;
        resp.body_size = 0;

        curl_slist_free_all(resolve);
        resolve = fixed_host_resolve(fixed, fixed->ips[1], url);
        if (resolve != NULL)
        {
            curl_easy_setopt(curl, CURLOPT_RESOLVE, resolve);
            watchdog_pet_network();
            curl_code = curl_easy_perform(curl);
        }
    }

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
