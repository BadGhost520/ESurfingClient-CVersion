#include "utils/PlatformUtils.h"
#include "utils/Logger.h"
#include "NetClient.h"
#include "States.h"

#include <openssl/evp.h>
#include <curl/curl.h>
#include <string.h>
#include <stdio.h>

#define SCHOOL_ID_LENGTH 8
#define DOMAIN_LENGTH 16
#define AREA_LENGTH 8

static const char s_req_content_type[] = "application/x-www-form-urlencoded";
static const char s_req_accept[] = "text/html,text/xml,application/xhtml+xml,application/x-javascript,*/*";
static const char s_generate_url[] = "http://connect.rom.miui.com/generate_204";

static char s_school_id[SCHOOL_ID_LENGTH];
static char s_domain[DOMAIN_LENGTH];
static char s_area[AREA_LENGTH];

char* extract_url_param(const char* url, const char* search_str_start)
{
    if (!url)
    {
        LOG_ERROR("URL 为空");
        return NULL;
    }
    const size_t len = strlen(search_str_start);
    char* search_pattern = malloc(len + 2);
    if (!search_pattern)
    {
        LOG_ERROR("分配内存失败");
        return NULL;
    }
    snprintf(search_pattern, len + 2, "%s=", search_str_start);
    char* result = extract_between_tags(url, search_pattern, "&");
    free(search_pattern);
    return result;
}

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
            snprintf(s_school_id, SCHOOL_ID_LENGTH, "%.*s", (uint8_t)valid_len, value);
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
            snprintf(s_domain, DOMAIN_LENGTH, "%.*s", (uint8_t)valid_len, value);
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
            snprintf(s_area, AREA_LENGTH, "%.*s", (uint8_t)valid_len, value);
            LOG_INFO("Area: %s", s_area);
        }
    }

    if (real_size >= 9 && strncasecmp(header, "Location:", 9) == 0)
    {
        if (!g_prog_status[thread_idx].runtime_status.last_location_lock)
        {
            const char* value = header + 9;
            while (*value == ' ') value++;
            const size_t valid_len = strcspn(value, "\r\n");
            snprintf(g_prog_status[thread_idx].last_location, LAST_LOCATION_LEN, "%.*s", (uint8_t)valid_len, value);
            LOG_VERBOSE("现在的 last_location: %s", g_prog_status[thread_idx].last_location);
        }
    }

    return real_size;
}

static size_t write_cb(const void* contents, const size_t size, const size_t nmemb, void* userdata)
{
    HTTPResponse* resp = userdata;
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

static CurlStatus create_post_client(CURL** curl, struct curl_slist** headers, HTTPResponse* response, const char* post_url, const char* post_data)
{
    *curl = curl_easy_init();
    if (!*curl) return CURL_INIT_FAILURE;

    curl_easy_setopt(*curl, CURLOPT_HTTPHEADER, *headers);
    curl_easy_setopt(*curl, CURLOPT_URL, post_url);
    curl_easy_setopt(*curl, CURLOPT_POSTFIELDS, post_data);
    curl_easy_setopt(*curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(*curl, CURLOPT_WRITEDATA, response);
    curl_easy_setopt(*curl, CURLOPT_TIMEOUT, 5L);
    curl_easy_setopt(*curl, CURLOPT_CONNECTTIMEOUT, 5L);

    if (g_use_cus_ip) curl_easy_setopt(*curl, CURLOPT_INTERFACE, g_prog_status[thread_idx].login_cfg.ip);

    return CURL_INIT_SUCCESS;
}

static CurlStatus create_get_client(CURL** curl, struct curl_slist** headers, HTTPResponse* response, const char* get_url)
{
    *curl = curl_easy_init();
    LOG_VERBOSE("curl_easy_init 完成, curl=%p", *curl);

    if (!*curl) {
        LOG_ERROR("curl 初始化失败");
        return CURL_INIT_FAILURE;
    }

    LOG_VERBOSE("设置选项");
    curl_easy_setopt(*curl, CURLOPT_HTTPHEADER, *headers);
    curl_easy_setopt(*curl, CURLOPT_URL, get_url);
    curl_easy_setopt(*curl, CURLOPT_HEADERFUNCTION, header_cb);
    curl_easy_setopt(*curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(*curl, CURLOPT_WRITEDATA, response);
    curl_easy_setopt(*curl, CURLOPT_TIMEOUT, 5L);
    curl_easy_setopt(*curl, CURLOPT_FOLLOWLOCATION, 0L);
    curl_easy_setopt(*curl, CURLOPT_MAXREDIRS, 5L);

    LOG_VERBOSE("g_use_cus_ip=%d", g_use_cus_ip);
    if (g_use_cus_ip) {
        LOG_VERBOSE("设置网络接口 %s", g_prog_status[thread_idx].login_cfg.ip);
        curl_easy_setopt(*curl, CURLOPT_INTERFACE, g_prog_status[thread_idx].login_cfg.ip);
    }

    return CURL_INIT_SUCCESS;
}

static void add_header(struct curl_slist** headers, const char* fmt, ...)
{
    va_list local_args;
    char header_buf[128];

    va_start(local_args, fmt);
    vsnprintf(header_buf, sizeof(header_buf), fmt, local_args);
    va_end(local_args);

    *headers = curl_slist_append(*headers, header_buf);
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
        return REQUEST_WARNING;
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

static HTTPResponse post(const char* url, const char* data, struct curl_slist* headers)
{
    LOG_VERBOSE("POST 地址: %s", url);
    LOG_VERBOSE("POST 数据: %s", data);

    CURL* curl;
    HTTPResponse resp = {0};

    if (create_post_client(&curl, &headers, &resp, url, data) == CURL_INIT_FAILURE)
    {
        resp.status = INIT_ERROR;
        return resp;
    }

    const CURLcode curl_code = curl_easy_perform(curl);
    if (curl_code != CURLE_OK)
    {
        curl_easy_cleanup(curl);
        resp.status = curl_err_msg_out(curl_code);
        return resp;
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    resp.status = REQUEST_SUCCESS;
    return resp;
}

static HTTPResponse get(const char* url, struct curl_slist* headers)
{
    CURL* curl;
    HTTPResponse resp = {0};
    if (create_get_client(&curl, &headers, &resp, url) == CURL_INIT_FAILURE)
    {
        LOG_ERROR("初始化错误");
        resp.status = INIT_ERROR;
        return resp;
    }
    LOG_VERBOSE("执行 CURL");
    CURLcode curl_code = curl_easy_perform(curl);
    if (curl_code != CURLE_OK)
    {
        curl_easy_cleanup(curl);
        resp.status = curl_err_msg_out(curl_code);
        return resp;
    }
    LOG_VERBOSE("获取响应码");
    long resp_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp_code);
    curl_easy_cleanup(curl);
    if (resp_code == 302)
    {
        LOG_VERBOSE("重定向至: %s", g_prog_status[thread_idx].last_location);
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
        LOG_VERBOSE("响应码: 204");
        resp.status = REQUEST_SUCCESS;
        return resp;
    }
    LOG_ERROR("HTTP 响应错误, 响应码: %d", resp_code);
    resp.status = REQUEST_ERROR;
    return resp;
}

HTTPResponse post_with_header(const char* url, const char* data)
{
    struct curl_slist* headers = NULL;
    HTTPResponse resp = {0};
    char* md5_hash = calc_md5(data);
    if (!md5_hash)
    {
        LOG_ERROR("计算 MD5 失败");
        resp.status = REQUEST_ERROR;
        return resp;
    }
    add_header(&headers, "CDC-Checksum: %s", md5_hash);
    free(md5_hash);
    add_header(&headers, "Content-Type: %s", s_req_content_type);
    add_header(&headers, "User-Agent: %s", g_prog_status[thread_idx].login_cfg.user_agent);
    add_header(&headers, "Accept: %s", s_req_accept);
    add_header(&headers, "Client-ID: %s", g_prog_status[thread_idx].auth_cfg.client_id);
    add_header(&headers, "Algo-ID: %s", g_prog_status[thread_idx].auth_cfg.algo_id);
    add_header(&headers, "CDC-SchoolId: %s", s_school_id);
    add_header(&headers, "CDC-Domain: %s", s_domain);
    add_header(&headers, "CDC-Area: %s", s_area);
    resp = post(url, data, headers);
    return resp;
}

HTTPResponse get_with_header(const char* url)
{
    struct curl_slist* headers = NULL;
    HTTPResponse resp = {0};
    add_header(&headers, "User-Agent: %s", g_prog_status[thread_idx].login_cfg.user_agent);
    add_header(&headers, "Accept: %s", s_req_accept);
    add_header(&headers, "Client-ID: %s", g_prog_status[thread_idx].auth_cfg.client_id);
    resp = get(url, headers);
    return resp;
}

static void get_school_ip_symbol()
{
    const char* school_ip = extract_url_param(g_prog_status[0].last_location, "wlanuserip");
    snprintf(school_network_symbol, SCHOOL_NETWORK_SYMBOL, "%s", extract_between_tags(school_ip, "", strchr(strchr(school_ip, '.') + 1, '.')));
    LOG_DEBUG("校园网标志: %s", school_network_symbol);
}

void get_last_location()
{
    for (thread_idx = 0; thread_idx < g_prog_cnt; thread_idx++)
    {
        refresh_states();
        HTTPResponse resp = {0};
        resp = get_with_header(s_generate_url);
        while (resp.status == REQUEST_REDIRECT) resp = get_with_header(g_prog_status[thread_idx].last_location);
        g_prog_status[thread_idx].runtime_status.last_location_lock = true;
        LOG_DEBUG("配置 %" PRIu8 " 获取认证配置 URL: %s", g_prog_status[thread_idx].login_cfg.idx, g_prog_status[thread_idx].last_location);
    }
    get_school_ip_symbol();
}

NetworkStatus check_network_status()
{
    const HTTPResponse resp = get_with_header(s_generate_url);
    return resp.status;
}

bool check_ip_validity(const char* ip)
{
    if (!g_use_cus_ip) return false;
    LOG_DEBUG("传入的 IP 地址: %s", ip);
    char cmd[256];
#ifdef _WIN32
    snprintf(cmd, sizeof(cmd), "ping -n 1 -w 1000 %s > nul 2>&1", ip);
#else
    snprintf(cmd, sizeof(cmd), "ping -c 1 -W 1 %s > /dev/null 2>&1", ip);
#endif
    LOG_DEBUG("组成的指令: %s", cmd);
    return system(cmd) == 0;
}
