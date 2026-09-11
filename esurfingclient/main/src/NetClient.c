#include "utils/PlatformUtils.h"
#include "utils/Logger.h"
#include "NetClient.h"
#include "States.h"

#include <openssl/evp.h>
#include <string.h>
#include <stdio.h>

#ifdef __OPENWRT__
#include <errno.h>

#ifndef SOL_SOCKET
    #define SOL_SOCKET 1
#endif

#ifndef SO_MARK
    #define SO_MARK 36
#endif
#endif

#define MAX_LEN 128

#define SCHOOL_ID_LENGTH 8
#define DOMAIN_LENGTH 16
#define AREA_LENGTH 8

#define REQ_CONTENT_TYPE "Content-Type: application/x-www-form-urlencoded"
#define REQ_ACCEPT "Accept: text/html,text/xml,application/xhtml+xml,application/x-javascript,*/*"
#define GENERATE_URL "http://connect.rom.miui.com/generate_204"
#define GENERATE_BAK_URL "http://1.1.1.1"
#define AUTH_IP "http://14.146.227.141:7001"
#define AUTH_BAK_IP "http://121.8.177.212:7001"

static char s_school_id[SCHOOL_ID_LENGTH];
static char s_domain[DOMAIN_LENGTH];
static char s_area[AREA_LENGTH];

static _Thread_local char s_request_url[LOCATION_LEN];

static void resolve_url(char* out, size_t out_len, const char* base, const char* ref)
{
    if (out == NULL || out_len == 0) return;
    out[0] = '\0';
    if (ref == NULL || ref[0] == '\0')
    {
        if (base) snprintf(out, out_len, "%s", base);
        return;
    }
    if (strncmp(ref, "http://", 7) == 0 || strncmp(ref, "https://", 8) == 0)
    {
        snprintf(out, out_len, "%s", ref);
        return;
    }
    if (base == NULL || base[0] == '\0')
    {
        snprintf(out, out_len, "%s", ref);
        return;
    }
    if (ref[0] == '/' && ref[1] == '/')
    {
        const char* scheme_end = strstr(base, "://");
        if (scheme_end) snprintf(out, out_len, "%.*s:%s", (int)(scheme_end - base), base, ref);
        else snprintf(out, out_len + 5, "http:%s", ref);
        return;
    }

    const char* scheme = strstr(base, "://");
    const char* host_start = scheme ? scheme + 3 : base;
    const char* path_start = strchr(host_start, '/');
    if (ref[0] == '/')
    {
        if (path_start) snprintf(out, out_len, "%.*s%s", (int)(path_start - base), base, ref);
        else snprintf(out, out_len, "%s%s", base, ref);
        return;
    }

    if (path_start)
    {
        const char* query = strchr(path_start, '?');
        const char* end = query ? query : path_start + strlen(path_start);
        const char* last_slash = path_start;
        for (const char* p = path_start; p < end; p++)
        {
            if (*p == '/') last_slash = p;
        }
        snprintf(out, out_len, "%.*s/%s", (int)(last_slash - base), base, ref);
    }
    else
    {
        snprintf(out, out_len, "%s/%s", base, ref);
    }
}

char* extract_url_param(const char* url, const char* search_str_start)
{
    if (url == NULL || search_str_start == NULL)
    {
        LOG_ERROR("URL 为空");
        return NULL;
    }

    const size_t name_len = strlen(search_str_start);
    char search_pattern[64];
    if (name_len + 2 > sizeof(search_pattern))
    {
        LOG_ERROR("参数名过长");
        return NULL;
    }
    snprintf(search_pattern, sizeof(search_pattern), "%s=", search_str_start);

    const char* start = strstr(url, search_pattern);
    if (start == NULL)
    {
        LOG_ERROR("未找到参数: %s", search_pattern);
        return NULL;
    }
    start += name_len + 1;

    /* 最后一个参数后面没有 '&', 也要能截取到结尾或 '#' */
    const size_t value_len = strcspn(start, "&#");
    char* result = malloc(value_len + 1);
    if (result == NULL)
    {
        LOG_ERROR("分配内存失败");
        return NULL;
    }
    memcpy(result, start, value_len);
    result[value_len] = '\0';
    return result;
}

#ifdef __OPENWRT__
static curl_socket_t open_socket_callback(void* client_p, curlsocktype purpose, struct curl_sockaddr* addr)
{
    (void)client_p;
    (void)purpose;
    curl_socket_t sock_fd = socket(addr->family, addr->socktype, addr->protocol);
    if (sock_fd == CURL_SOCKET_BAD)
    {
        LOG_ERROR("创建 socket 失败: %s", strerror(errno));
        return CURL_SOCKET_BAD;
    }

    if (g_prog_status[tl_thread_idx].login_cfg.mark != 0)
    {
        if (setsockopt(sock_fd, SOL_SOCKET, SO_MARK, &g_prog_status[tl_thread_idx].login_cfg.mark, sizeof(g_prog_status[tl_thread_idx].login_cfg.mark)) == -1)
        {
            LOG_ERROR("设置 SO_MARK 失败 (mark = %" PRIu32 " (0x%x)): %s", g_prog_status[tl_thread_idx].login_cfg.mark, g_prog_status[tl_thread_idx].login_cfg.mark, strerror(errno));
        }
        else
        {
            LOG_VERBOSE("设置 SO_MARK = %" PRIu32 " (0x%x)", g_prog_status[tl_thread_idx].login_cfg.mark, g_prog_status[tl_thread_idx].login_cfg.mark);
        }
    }

    return sock_fd;
}
#endif

static size_t header_cb(const void* contents, const size_t size, const size_t nmemb, void* userdata)
{
    const size_t real_size = size * nmemb;
    const char* header = contents;

    if (real_size >= 9 && strncmp(header, "schoolid:", 9) == 0 && !s_school_id[0])
    {
        if (s_school_id[0] == '\0')
        {
            LOG_VERBOSE("原始数据: %s", header);

            const char* value = header + 9;
            while (*value == ' ') value++;
            const size_t valid_len = strcspn(value, "\r\n");

            size_t copy_len = valid_len;
            if (copy_len >= SCHOOL_ID_LENGTH)
            {
                copy_len = SCHOOL_ID_LENGTH - 1;
                LOG_WARN("School Id 被截断, 原长度: %zu, 缓冲区大小: %d", valid_len, SCHOOL_ID_LENGTH);
            }

            memcpy(s_school_id, value, copy_len);
            s_school_id[copy_len] = '\0';

            LOG_INFO("School Id: %s", s_school_id);
        }
    }

    if (real_size >= 7 && strncmp(header, "domain:", 7) == 0 && !s_domain[0])
    {
        if (s_domain[0] == '\0')
        {
            LOG_VERBOSE("原始数据: %s", header);

            const char* value = header + 7;
            while (*value == ' ') value++;
            const size_t valid_len = strcspn(value, "\r\n");

            size_t copy_len = valid_len;
            if (copy_len >= DOMAIN_LENGTH)
            {
                copy_len = DOMAIN_LENGTH - 1;
                LOG_WARN("Domain 被截断, 原长度: %zu, 缓冲区大小: %d", valid_len, DOMAIN_LENGTH);
            }

            memcpy(s_domain, value, copy_len);
            s_domain[copy_len] = '\0';

            LOG_INFO("Domain: %s", s_domain);
        }
    }

    if (real_size >= 5 && strncmp(header, "area:", 5) == 0 && !s_area[0])
    {
        if (s_area[0] == '\0')
        {
            LOG_VERBOSE("原始数据: %s", header);

            const char* value = header + 5;
            while (*value == ' ') value++;
            const size_t valid_len = strcspn(value, "\r\n");

            size_t copy_len = valid_len;
            if (copy_len >= AREA_LENGTH)
            {
                copy_len = AREA_LENGTH - 1;
                LOG_WARN("Area 被截断, 原长度: %zu, 缓冲区大小: %d", valid_len, AREA_LENGTH);
            }

            memcpy(s_area, value, copy_len);
            s_area[copy_len] = '\0';

            LOG_INFO("Area: %s", s_area);
        }
    }

    if (real_size >= 9 && strncasecmp(header, "Location:", 9) == 0)
    {
        if (tl_thread_idx > -1)
        {
            if (!g_prog_status[tl_thread_idx].last_location_lock)
            {
                LOG_VERBOSE("原始数据: %s", header);

                const char* value = header + 9;
                while (*value == ' ') value++;
                const size_t valid_len = strcspn(value, "\r\n");

                size_t copy_len = valid_len;
                if (copy_len >= LOCATION_LEN)
                {
                    copy_len = LOCATION_LEN - 1;
                    LOG_WARN("Location 被截断, 原长度: %zu, 缓冲区大小: %d", valid_len, LOCATION_LEN);
                }

                char location[LOCATION_LEN];
                memcpy(location, value, copy_len);
                location[copy_len] = '\0';

                char resolved[LAST_LOCATION_LEN];
                resolve_url(resolved, sizeof(resolved), s_request_url, location);
                snprintf(g_prog_status[tl_thread_idx].last_location, LAST_LOCATION_LEN, "%s", resolved);

                LOG_VERBOSE("现在的 last_location: %s (长度: %zu)",
                            g_prog_status[tl_thread_idx].last_location,
                            strlen(g_prog_status[tl_thread_idx].last_location));
            }
        }
    }

    return real_size;
}

static size_t write_cb(const void* contents, const size_t size, const size_t nmemb, void* userdata)
{
    curl_resp_t* resp = userdata;
    const size_t real_size = size * nmemb;
    char* ptr = realloc(resp->body_data, resp->body_size + real_size + 1);

    if (!ptr) return 0;

    resp->body_data = ptr;
    memcpy(&resp->body_data[resp->body_size], contents, real_size);
    resp->body_size += real_size;
    resp->body_data[resp->body_size] = 0;

    return real_size;
}

static char* calc_md5(const char* data)
{
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len;
    char* md5_str = malloc(33);

    if (!md5_str)
    {
        LOG_ERROR("分配内存失败");
        return NULL;
    }

    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (!mdctx)
    {
        free(md5_str);
        return NULL;
    }

    const EVP_MD* md = EVP_md5();
    if (EVP_DigestInit_ex(mdctx, md, NULL) != 1)
    {
        EVP_MD_CTX_free(mdctx);
        free(md5_str);
        return NULL;
    }

    if (EVP_DigestUpdate(mdctx, data, strlen(data)) != 1)
    {
        EVP_MD_CTX_free(mdctx);
        free(md5_str);
        return NULL;
    }

    if (EVP_DigestFinal_ex(mdctx, digest, &digest_len) != 1)
    {
        EVP_MD_CTX_free(mdctx);
        free(md5_str);
        return NULL;
    }
    EVP_MD_CTX_free(mdctx);

    for (unsigned int i = 0; i < digest_len; i++) sprintf(&md5_str[i*2], "%02x", (unsigned int)digest[i]);

    return md5_str;
}

static network_status_t curl_err_msg_out(const CURLcode curl_code)
{
    switch (curl_code)
    {
    case CURLE_COULDNT_RESOLVE_HOST:
        LOG_ERROR("curl 错误码: 6, 错误原因: DNS 解析错误");
        return STATUS_ERROR;
    case CURLE_COULDNT_CONNECT:
        LOG_ERROR("curl 错误码: 7, 错误原因: 连接服务器失败");
        return STATUS_ERROR;
    case CURLE_OPERATION_TIMEDOUT:
        LOG_ERROR("curl 错误码: 28, 错误原因: 操作超时");
        return STATUS_ERROR;
    case CURLE_HTTP_RETURNED_ERROR:
        LOG_ERROR("curl 错误码: 22, 错误原因: HTTP 状态码 ≥ 400");
        return STATUS_ERROR;
    case CURLE_GOT_NOTHING:
        LOG_ERROR("curl 错误码: 52, 错误原因: 服务器返回空数据");
        return STATUS_ERROR;
    case CURLE_URL_MALFORMAT:
        LOG_ERROR("curl 错误码: 3, 错误原因: URL 格式错误");
        return STATUS_ERROR;
    case CURLE_WRITE_ERROR:
        LOG_ERROR("curl 错误码: 23, 错误原因: 写入数据失败");
        return STATUS_ERROR;
    case CURLE_ABORTED_BY_CALLBACK:
        LOG_ERROR("curl 错误码: 42, 错误原因: 回调函数中止");
        return STATUS_ERROR;
    default:
        LOG_ERROR("未知错误");
        return STATUS_ERROR;
    }
}

curl_resp_t post(const char* url, const char* data)
{
    LOG_VERBOSE("POST 地址: %s", url);
    LOG_VERBOSE("POST 数据: %s", data);

    curl_resp_t resp = {0};

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
    LOG_VERBOSE("POST 添加头 %s", ua);
    LOG_VERBOSE("POST 添加头 %s", REQ_ACCEPT);
    LOG_VERBOSE("POST 添加头 %s", c_id);
    LOG_VERBOSE("POST 添加头 %s", a_id);
    LOG_VERBOSE("POST 添加头 %s", cdc_sid);
    LOG_VERBOSE("POST 添加头 %s", cdc_d);
    LOG_VERBOSE("POST 添加头 %s", cdc_a);
    LOG_VERBOSE("下标: %" PRId8, tl_thread_idx);

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
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3L);
    // 总超时时长
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp);

#ifdef __OPENWRT__
    curl_easy_setopt(curl, CURLOPT_OPENSOCKETFUNCTION, open_socket_callback);
#endif

    LOG_VERBOSE("执行 CURL");
    const CURLcode curl_code = curl_easy_perform(curl);
    if (curl_code != CURLE_OK)
    {
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
        LOG_VERBOSE("无内容, 响应码: 204");
        resp.http_code = HTTP_NO_CONTENT;
        resp.status = STATUS_OK;
        return resp;
    }
    if (resp_code == 302)
    {
        LOG_DEBUG("重定向, 响应码: 302");
        if (tl_thread_idx > -1) LOG_VERBOSE("重定向至: %s", g_prog_status[tl_thread_idx].last_location);
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
    LOG_VERBOSE("GET 地址: %s", url);

    curl_resp_t resp = {0};

    char ua[MAX_LEN] = {0};
    char c_id[MAX_LEN] = {0};

    struct curl_slist* headers = NULL;

    if (tl_thread_idx > -1)
    {
        snprintf(s_request_url, sizeof(s_request_url), "%s", safe_str(url));
        snprintf(ua, MAX_LEN, "User-Agent: %s", safe_str(g_prog_status[tl_thread_idx].login_cfg.user_agent));
        snprintf(c_id, MAX_LEN, "Client-ID: %s", safe_str(g_prog_status[tl_thread_idx].auth_cfg.client_id));

        LOG_VERBOSE("GET 添加头 %s", ua);
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
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3L);
    // 总超时时长
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    // 是否跟随重定向 (当前否)
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);
    if (connect_only) // 判断是否仅连接 (检测网络状态用)
    {
        curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 2L);
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
    const CURLcode curl_code = curl_easy_perform(curl);
    if (curl_code != CURLE_OK)
    {
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
        if (tl_thread_idx > -1) LOG_VERBOSE("重定向至: %s", g_prog_status[tl_thread_idx].last_location);
        resp.http_code = HTTP_FOUND;
        resp.status = STATUS_NEED_AUTH;
        return resp;
    }

    LOG_ERROR("意外的 HTTP 响应码: %ld", resp_code);
    resp.http_code = resp_code;
    resp.status = STATUS_ERROR;
    return resp;
}

network_status_t check_network_status(const bool connect_only)
{
    curl_resp_t resp = {0};
    connection_status_t conn_status = 0;

    /*
     * miui generate_204 URL
     * 204 正常联网
     * 302 需要认证
     * 其他则是非正常状态
     */

    resp = get(GENERATE_URL, connect_only);

    conn_status = CONNECT_INTERNET;

    if (resp.curl_code != CURLE_OK) // 主检测 URL 无法连通
    {
        /*
         * dns 错误时备用方案
         * http://1.1.1.1
         * 301 正常联网
         * 302 需要认证
         * 其他则是非正常状态
         */

        LOG_WARN("主检测 URL 无法连通, 切换到备用 IP 地址 URL");
        resp = get(GENERATE_BAK_URL, connect_only);

        conn_status = CONNECT_INTERNET;

        if (resp.curl_code != CURLE_OK) // 备用 IP URL 无法连通
        {
            /*
             * 外部网络无法连通, 进一步检查能否连通认证服务器
             * 200 正常连通
             * 302 可能是需要认证
             * 其他则是非正常状态
             */

            LOG_ERROR("备用 IP 地址 URL 无法连通, 初步判定为无法连通外部互联网");
            LOG_INFO("正在检测认证服务器连通性");

            conn_status = CONNECT_AUTH_SERVER;

            resp = get(AUTH_IP, connect_only);

            if (resp.curl_code != CURLE_OK) // 认证服务器 1 无法连通
            {
                /*
                 * 认证服务器 1 无法连通, 检测认证服务器 2
                 * 200 正常连通
                 * 302 可能是需要认证
                 * 其他则是非正常状态
                 */

                LOG_WARN("认证服务器 1 无法连通, 正在检测认证服务器 2 连通性");

                resp = get(AUTH_BAK_IP, connect_only);

                conn_status = CONNECT_AUTH_SERVER;

                if (resp.curl_code != CURLE_OK) // 认证服务器 2 无法连通
                {
                    /*
                     * 认证服务器 2 无法连通, 确认为网络连接错误
                     */

                    LOG_ERROR("认证服务器 2 无法连通, 建议检查外部网络连接情况");

                    conn_status = CONNECT_ERROR;
                }
            }
        }
    }

    if (conn_status == CONNECT_INTERNET && (resp.http_code == HTTP_NO_CONTENT || resp.http_code == HTTP_MOVED_PERMANENTLY))
    {
        // 正常联网
        return STATUS_OK;
    }
    if (resp.http_code == HTTP_FOUND)
    {
        // 需要认证
        return STATUS_NEED_AUTH;
    }
    // 网络错误
    return STATUS_ERROR;
}

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
        if (g_need_exit)
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
