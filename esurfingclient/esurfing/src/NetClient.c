#include "utils/PlatformUtils.h"
#include "utils/Shutdown.h"
#include "utils/Logger.h"
#include "NetClient.h"
#include "States.h"

#include <openssl/evp.h>
#include <string.h>
#include <stdio.h>

#ifdef __OPENWRT__
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

static const char s_req_content_type[] = "Content-Type: application/x-www-form-urlencoded";
static const char s_req_accept[] = "Accept: text/html,text/xml,application/xhtml+xml,application/x-javascript,*/*";
static const char s_generate_url[] = "http://connect.rom.miui.com/generate_204";

static char s_school_id[SCHOOL_ID_LENGTH];
static char s_domain[DOMAIN_LENGTH];
static char s_area[AREA_LENGTH];

char* extract_url_param(const char* url, const char* search_str_start)
{
    if (url == NULL)
    {
        LOG_ERROR("URL 为空");
        return NULL;
    }
    const size_t len = strlen(search_str_start);
    char* search_pattern = malloc(len + 2);
    if (search_pattern == NULL)
    {
        LOG_ERROR("分配内存失败");
        return NULL;
    }
    snprintf(search_pattern, len + 2, "%s=", search_str_start);
    char* result = extract_between_tags(url, search_pattern, "&");
    free(search_pattern);
    return result;
}

#ifdef __OPENWRT__
static curl_socket_t open_socket_callback(void* client_p, curlsocktype purpose, struct curl_sockaddr* addr)
{
    curl_socket_t sock_fd = socket(addr->family, addr->socktype, addr->protocol);
    if (sock_fd == CURL_SOCKET_BAD)
    {
        LOG_ERROR("创建 socket 失败: %s", strerror(errno));
        return CURL_SOCKET_BAD;
    }

    if (g_prog_status[thread_idx].mark != 0)
    {
        if (setsockopt(sock_fd, SOL_SOCKET, SO_MARK, &g_prog_status[thread_idx].mark, sizeof(g_prog_status[thread_idx].mark)) == -1)
        {
            LOG_ERROR("设置 SO_MARK 失败 (mark = %d): %s", g_prog_status[thread_idx].mark, strerror(errno));
        }
        else
        {
            LOG_DEBUG("设置 SO_MARK = %d (0x%x)", g_prog_status[thread_idx].mark, g_prog_status[thread_idx].mark);
        }
    }

    return sock_fd;
}
#endif

static size_t header_cb(const void *contents, const size_t size, const size_t nmemb, void* userdata)
{
    const size_t real_size = size * nmemb;
    const char* header = contents;

    if (real_size >= 9 && strncmp(header, "schoolid:", 9) == 0 && !s_school_id[0])
    {
        if (s_school_id[0] == '\0')
        {
            const char* value = header + 9;
            while (*value == ' ') value++;
            const size_t valid_len = strcspn(value, "\r\n");
            snprintf(s_school_id, SCHOOL_ID_LENGTH, "%.*s", (uint8_t)valid_len, safe_str(value));
            LOG_INFO("School Id: %s", s_school_id);
        }
    }

    if (real_size >= 7 && strncmp(header, "domain:", 7) == 0 && !s_domain[0])
    {
        if (s_domain[0] == '\0')
        {
            const char* value = header + 7;
            while (*value == ' ') value++;
            const size_t valid_len = strcspn(value, "\r\n");
            snprintf(s_domain, DOMAIN_LENGTH, "%.*s", (uint8_t)valid_len, safe_str(value));
            LOG_INFO("Domain: %s", s_domain);
        }
    }

    if (real_size >= 5 && strncmp(header, "area:", 5) == 0 && !s_area[0])
    {
        if (s_area[0] == '\0')
        {
            const char* value = header + 5;
            while (*value == ' ') value++;
            const size_t valid_len = strcspn(value, "\r\n");
            snprintf(s_area, AREA_LENGTH, "%.*s", (uint8_t)valid_len, safe_str(value));
            LOG_INFO("Area: %s", s_area);
        }
    }

    if (real_size >= 9 && strncasecmp(header, "Location:", 9) == 0)
    {
        if (thread_idx != -1)
        {
            if (!g_prog_status[thread_idx].runtime_status.last_location_lock)
            {
                const char* value = header + 9;
                while (*value == ' ') value++;
                const size_t valid_len = strcspn(value, "\r\n");
                snprintf(g_prog_status[thread_idx].last_location, LAST_LOCATION_LEN, "%.*s", (uint8_t)valid_len, safe_str(value));
                LOG_VERBOSE("现在的 last_location: %s", g_prog_status[thread_idx].last_location);
            }
        }
    }

    return real_size;
}

static size_t write_cb(const void* contents, const size_t size, const size_t nmemb, void* userdata)
{
    http_resp_t* resp = userdata;
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

static NetworkStatus curl_err_msg_out(const CURLcode curl_code)
{
    switch (curl_code)
    {
    case CURLE_COULDNT_RESOLVE_HOST:
        LOG_ERROR("curl 错误码: 6, 错误原因: DNS 解析错误");
        return REQUEST_ERROR;
    case CURLE_COULDNT_CONNECT:
        LOG_ERROR("curl 错误码: 7, 错误原因: 连接服务器失败");
        return REQUEST_ERROR;
    case CURLE_OPERATION_TIMEDOUT:
        LOG_ERROR("curl 错误码: 28, 错误原因: 操作超时");
        return REQUEST_WARN;
    case CURLE_HTTP_RETURNED_ERROR:
        LOG_ERROR("curl 错误码: 22, 错误原因: HTTP 状态码 ≥ 400");
        return REQUEST_ERROR;
    case CURLE_GOT_NOTHING:
        LOG_ERROR("curl 错误码: 52, 错误原因: 服务器返回空数据");
        return REQUEST_ERROR;
    case CURLE_URL_MALFORMAT:
        LOG_ERROR("curl 错误码: 3, 错误原因: URL 格式错误");
        return REQUEST_ERROR;
    case CURLE_WRITE_ERROR:
        LOG_ERROR("curl 错误码: 23, 错误原因: 写入数据失败");
        return REQUEST_ERROR;
    case CURLE_ABORTED_BY_CALLBACK:
        LOG_ERROR("curl 错误码: 42, 错误原因: 回调函数中止");
        return REQUEST_ERROR;
    default:
        LOG_ERROR("未知错误");
        return REQUEST_ERROR;
    }
}

http_resp_t post(const char* url, const char* data)
{
    LOG_VERBOSE("POST 地址: %s", url);
    LOG_VERBOSE("POST 数据: %s", data);

    http_resp_t resp = {0};

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
        resp.status = REQUEST_ERROR;
        return resp;
    }

    snprintf(md5_hash_str, MAX_LEN, "CDC-Checksum: %s", safe_str(md5_hash));
    free(md5_hash);
    snprintf(ua, MAX_LEN, "User-Agent: %s", safe_str(g_prog_status[thread_idx].login_cfg.user_agent));
    snprintf(c_id, MAX_LEN, "Client-ID: %s", safe_str(g_prog_status[thread_idx].auth_cfg.client_id));
    snprintf(a_id, MAX_LEN, "Algo-ID: %s", safe_str(g_prog_status[thread_idx].auth_cfg.algo_id));
    snprintf(cdc_sid, MAX_LEN, "CDC-SchoolId: %s", safe_str(s_school_id));
    snprintf(cdc_d, MAX_LEN, "CDC-Domain: %s", safe_str(s_domain));
    snprintf(cdc_a, MAX_LEN, "CDC-Area: %s", safe_str(s_area));

    LOG_VERBOSE("POST 添加头 %s", md5_hash_str);
    LOG_VERBOSE("POST 添加头 %s", s_req_content_type);
    LOG_VERBOSE("POST 添加头 %s", ua);
    LOG_VERBOSE("POST 添加头 %s", s_req_accept);
    LOG_VERBOSE("POST 添加头 %s", c_id);
    LOG_VERBOSE("POST 添加头 %s", a_id);
    LOG_VERBOSE("POST 添加头 %s", cdc_sid);
    LOG_VERBOSE("POST 添加头 %s", cdc_d);
    LOG_VERBOSE("POST 添加头 %s", cdc_a);
    LOG_VERBOSE("下标: %" PRId8, thread_idx);

    struct curl_slist* headers = NULL;

    headers = curl_slist_append(headers, md5_hash_str);
    headers = curl_slist_append(headers, s_req_content_type);
    headers = curl_slist_append(headers, ua);
    headers = curl_slist_append(headers, s_req_accept);
    headers = curl_slist_append(headers, c_id);
    headers = curl_slist_append(headers, a_id);
    headers = curl_slist_append(headers, cdc_sid);
    headers = curl_slist_append(headers, cdc_d);
    headers = curl_slist_append(headers, cdc_a);

    CURL* curl = curl_easy_init();
    if (curl == NULL)
    {
        LOG_ERROR("curl 初始化失败");
        resp.status = REQUEST_INIT_ERROR;
        curl_slist_free_all(headers);
        return resp;
    }
    LOG_VERBOSE("curl 初始化完成, curl=%p", curl);

    LOG_VERBOSE("设置 curl 选项");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);

#ifdef __OPENWRT__
    curl_easy_setopt(curl, CURLOPT_OPENSOCKETFUNCTION, open_socket_callback);
#endif

    LOG_VERBOSE("执行 CURL");
    const CURLcode curl_code = curl_easy_perform(curl);

    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);

    if (curl_code != CURLE_OK)
    {
        resp.status = curl_err_msg_out(curl_code);
        return resp;
    }

    resp.status = REQUEST_SUCCESS;
    return resp;
}

http_resp_t get(const char* url)
{
    LOG_VERBOSE("GET 地址: %s", url);

    http_resp_t resp = {0};

    char ua[MAX_LEN] = {0};
    char c_id[MAX_LEN] = {0};

    struct curl_slist* headers = NULL;

    if (thread_idx != -1)
    {
        snprintf(ua, MAX_LEN, "User-Agent: %s", safe_str(g_prog_status[thread_idx].login_cfg.user_agent));
        snprintf(c_id, MAX_LEN, "Client-ID: %s", safe_str(g_prog_status[thread_idx].auth_cfg.client_id));

        LOG_VERBOSE("GET 添加头 %s", ua);
        LOG_VERBOSE("GET 添加头 %s", s_req_accept);
        LOG_VERBOSE("GET 添加头 %s", c_id);
        LOG_VERBOSE("线程下标: %" PRId8, thread_idx);

        headers = curl_slist_append(headers, ua);
        headers = curl_slist_append(headers, s_req_accept);
        headers = curl_slist_append(headers, c_id);
    }

    CURL* curl = curl_easy_init();
    if (curl == NULL)
    {
        LOG_ERROR("curl 初始化失败");
        resp.status = REQUEST_INIT_ERROR;
        curl_slist_free_all(headers);
        return resp;
    }
    LOG_VERBOSE("curl 初始化完成, curl = %p", curl);

    LOG_VERBOSE("设置 curl 选项");
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);
    if (thread_idx != -1)
    {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_cb);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp);
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);
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
        return resp;
    }

    LOG_VERBOSE("获取响应码");
    long resp_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp_code);

    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);

    if (resp_code == 302)
    {
        LOG_DEBUG("重定向, 响应码: 302");
        if (thread_idx != -1) LOG_VERBOSE("重定向至: %s", g_prog_status[thread_idx].last_location);
        resp.status = REQUEST_REDIRECT;
        return resp;
    }
    if (resp_code == 200)
    {
        LOG_DEBUG("有响应体, 响应码: 200");
        resp.status = REQUEST_HAVE_RES;
        return resp;
    }
    if (resp_code == 204)
    {
        LOG_VERBOSE("无响应体, 响应码: 204");
        resp.status = REQUEST_SUCCESS;
        return resp;
    }

    LOG_ERROR("HTTP 响应错误, 响应码: %d", resp_code);
    resp.status = REQUEST_ERROR;
    return resp;
}

NetworkStatus check_network_status()
{
    const http_resp_t resp = get(s_generate_url);
    return resp.status;
}

static void get_school_ip_symbol()
{
    const char* school_ip = extract_url_param(g_prog_status[0].last_location, "wlanuserip");
    snprintf(school_network_symbol, SCHOOL_NETWORK_SYMBOL, "%s", safe_str(extract_between_tags(school_ip, "", strchr(strchr(school_ip, '.') + 1, '.'))));
    LOG_DEBUG("校园网标志: %s", school_network_symbol);
}

void get_last_location()
{
    http_resp_t resp = {0};

    uint8_t retry = 1;
    do
    {
        resp = get(s_generate_url);
        switch (resp.status)
        {
        case REQUEST_REDIRECT:
            break;
        default:
            if (retry > 5)
            {
                LOG_FATAL("超过重试次数, 退出程序");
                shut(1);
            }
            LOG_WARN("网络不可重定向, 重试: 第 %" PRIu8 " 次, 最多 5 次", retry);
            retry++;
            sleep_ms(1000);
            break;
        }
    } while (resp.status != REQUEST_REDIRECT);

    while (resp.status == REQUEST_REDIRECT) resp = get(g_prog_status[thread_idx].last_location);

    g_prog_status[thread_idx].runtime_status.last_location_lock = true;
    LOG_DEBUG("配置 %" PRIu8 " 获取认证配置 URL: %s", g_prog_status[thread_idx].login_cfg.idx, g_prog_status[thread_idx].last_location);

    get_school_ip_symbol(); // 获取校园网特征
}
