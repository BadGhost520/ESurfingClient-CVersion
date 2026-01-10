#include <openssl/evp.h>
#include <curl/curl.h>
#include <string.h>
#include <stdio.h>

#include "headFiles/utils/PlatformUtils.h"
#include "headFiles/utils/Logger.h"
#include "headFiles/NetClient.h"
#include "headFiles/States.h"

static const char request_content_type[] = "application/x-www-form-urlencoded";
static const char request_accept[] = "text/html,text/xml,application/xhtml+xml,application/x-javascript,*/*";
static const char generate_url[] = "http://connect.rom.miui.com/generate_204";
static char latest_location[LOCATION_LENGTH];
static char check_location[LOCATION_LENGTH];
static char school_id[SCHOOL_ID_LENGTH];
static char domain[DOMAIN_LENGTH];
static char area[AREA_LENGTH];

char last_location[LOCATION_LENGTH];
char school_network_symbol[SCHOOL_NETWORK_SYMBOL];

static __thread char thread_last_location[LOCATION_LENGTH];

static char* extractUrlParam(const char* url, const char* search_string_start)
{
    if (!url)
    {
        LOG_ERROR("URL 为空");
        return NULL;
    }
    const size_t len = strlen(search_string_start);
    char* search_pattern = malloc(len + 2);
    if (!search_pattern)
    {
        LOG_ERROR("分配内存失败");
        return NULL;
    }
    snprintf(search_pattern, len + 2, "%s=", search_string_start);
    char* result = extractBetweenTags(url, search_pattern, "&");
    free(search_pattern);
    return result;
}

static size_t headerCallback(const void *contents, const size_t size, const size_t nmemb, void* userdata)
{
    const size_t real_size = size * nmemb;
    const char* header = contents;
    if (real_size >= 9 && strncmp(header, "schoolid:", 9) == 0 && !school_id[0])
    {
        const char* value = header + 9;
        while (*value == ' ') value++;
        const size_t valid_len = strcspn(value, "\r\n");
        snprintf(school_id, SCHOOL_ID_LENGTH, "%.*s", (int)valid_len, value);
    }
    if (real_size >= 7 && strncmp(header, "domain:", 7) == 0 && !domain[0])
    {
        const char* value = header + 7;
        while (*value == ' ') value++;
        const size_t valid_len = strcspn(value, "\r\n");
        snprintf(domain, DOMAIN_LENGTH, "%.*s", (int)valid_len, value);
    }
    if (real_size >= 5 && strncmp(header, "area:", 5) == 0 && !area[0])
    {
        const char* value = header + 5;
        while (*value == ' ') value++;
        const size_t valid_len = strcspn(value, "\r\n");
        snprintf(area, AREA_LENGTH, "%.*s", (int)valid_len, value);
    }
    if (real_size >= 9 && strncasecmp(header, "Location:", 9) == 0)
    {
        const char* value = header + 9;
        while (*value == ' ') value++;
        const size_t valid_len = strcspn(value, "\r\n");
        snprintf(latest_location, LOCATION_LENGTH, "%.*s", (int)valid_len, value);
        snprintf(check_location, LOCATION_LENGTH, "%.*s", (int)valid_len, value);
    }
    return real_size;
}

static size_t writeCallback(const void* contents, const size_t size, const size_t nmemb, void* userdata)
{
    HTTPResponse* response = userdata;
    const size_t real_size = size * nmemb;
    char* ptr = realloc(response->body_data, response->body_size + real_size + 1);
    if (!ptr) return 0;
    response->body_data = ptr;
    memcpy(&response->body_data[response->body_size], contents, real_size);
    response->body_size += real_size;
    response->body_data[response->body_size] = 0;
    return real_size;
}

static char* calculateMD5(const char* data)
{
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digestLen;
    char* MD5String = malloc(33);
    if (!MD5String)
    {
        LOG_ERROR("分配内存失败");
        return NULL;
    }
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (!mdctx)
    {
        free(MD5String);
        return NULL;
    }
    const EVP_MD* md = EVP_md5();
    if (EVP_DigestInit_ex(mdctx, md, NULL) != 1)
    {
        EVP_MD_CTX_free(mdctx);
        free(MD5String);
        return NULL;
    }
    if (EVP_DigestUpdate(mdctx, data, strlen(data)) != 1)
    {
        EVP_MD_CTX_free(mdctx);
        free(MD5String);
        return NULL;
    }
    if (EVP_DigestFinal_ex(mdctx, digest, &digestLen) != 1)
    {
        EVP_MD_CTX_free(mdctx);
        free(MD5String);
        return NULL;
    }
    EVP_MD_CTX_free(mdctx);
    for (unsigned int i = 0; i < digestLen; i++) sprintf(&MD5String[i*2], "%02x", (unsigned int)digest[i]);
    return MD5String;
}

static CurlStatus createPostCurlClient(CURL** curl, struct curl_slist** headers, HTTPResponse* response, const char* post_url, const char* post_data)
{
    *curl = curl_easy_init();
    if (!*curl) return CURL_INIT_FAILURE;
    curl_easy_setopt(*curl, CURLOPT_HTTPHEADER, *headers);
    curl_easy_setopt(*curl, CURLOPT_URL, post_url);
    curl_easy_setopt(*curl, CURLOPT_POSTFIELDS, post_data);
    curl_easy_setopt(*curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(*curl, CURLOPT_WRITEDATA, response);
    curl_easy_setopt(*curl, CURLOPT_TIMEOUT, 5L);
    curl_easy_setopt(*curl, CURLOPT_CONNECTTIMEOUT, 5L);
    return CURL_INIT_SUCCESS;
}

static CurlStatus createGetCurlClient(CURL** curl, struct curl_slist** headers, HTTPResponse* response, const char* get_url)
{
    *curl = curl_easy_init();
    if (!*curl) return CURL_INIT_FAILURE;
    curl_easy_setopt(*curl, CURLOPT_HTTPHEADER, *headers);
    curl_easy_setopt(*curl, CURLOPT_URL, get_url);
    curl_easy_setopt(*curl, CURLOPT_HEADERFUNCTION, headerCallback);
    curl_easy_setopt(*curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(*curl, CURLOPT_WRITEDATA, response);
    curl_easy_setopt(*curl, CURLOPT_TIMEOUT, 5L);
    curl_easy_setopt(*curl, CURLOPT_FOLLOWLOCATION, 0L);
    curl_easy_setopt(*curl, CURLOPT_MAXREDIRS, 5L);
    return CURL_INIT_SUCCESS;
}

static void addCurlHeader(struct curl_slist** headers, const char* format, ...)
{
    va_list args;
    char header_buffer[128];
    va_start(args, format);
    vsnprintf(header_buffer, sizeof(header_buffer), format, args);
    va_end(args);
    *headers = curl_slist_append(*headers, header_buffer);
}

static HTTPResponse post(const char* url, const char* data, struct curl_slist* headers)
{
    LOG_DEBUG("POST 地址: %s", url);
    LOG_DEBUG("POST 数据: %s", data);
    CURL* curl;
    HTTPResponse response = {0};
    if (createPostCurlClient(&curl, &headers, &response, url, data) == CURL_INIT_FAILURE)
    {
        response.status = INIT_ERROR;
        return response;
    }
    const CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        response.status = REQUEST_ERROR;
        LOG_ERROR("网络错误，原因: %s",curl_easy_strerror(res));
        return response;
    }
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    response.status = REQUEST_SUCCESS;
    return response;
}

static HTTPResponse get(const char* url, struct curl_slist* headers)
{
    LOG_VERBOSE("GET 地址: %s", url);
    CURL* curl;
    HTTPResponse response = {0};
    if (createGetCurlClient(&curl, &headers, &response, url) == CURL_INIT_FAILURE)
    {
        response.status = INIT_ERROR;
        return response;
    }
    const CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        const char* error_msg = curl_easy_strerror(res);
        LOG_ERROR("HTTP 请求错误: %s (错误码: %d)", error_msg, res);
        curl_easy_cleanup(curl);
        if (res == 28) response.status = REQUEST_WARNING;
        else response.status = REQUEST_ERROR;
        return response;
    }
    long response_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    curl_easy_cleanup(curl);
    if (response_code == 302)
    {
        LOG_VERBOSE("重定向至: %s", latest_location);
        response.status = REQUEST_REDIRECT;
        return response;
    }
    if (response_code == 200)
    {
        LOG_DEBUG("有响应体");
        response.status = REQUEST_HAVE_RES;
        return response;
    }
    if (response_code == 204)
    {
        response.status = REQUEST_SUCCESS;
        return response;
    }
    LOG_ERROR("HTTP 响应错误, 响应码: %d", response_code);
    response.status = REQUEST_ERROR;
    return response;
}

HTTPResponse sessionPost(const char* url, const char* data)
{
    struct curl_slist* headers = NULL;
    HTTPResponse response = {0};
    char* MD5Hash = calculateMD5(data);
    if (!MD5Hash)
    {
        LOG_ERROR("计算 MD5 失败");
        response.status = REQUEST_ERROR;
        return response;
    }
    addCurlHeader(&headers, "CDC-Checksum: %s", MD5Hash);
    free(MD5Hash);
    addCurlHeader(&headers, "Content-Type: %s", request_content_type);
    addCurlHeader(&headers, "User-Agent: %s", thread_status[thread_index].dialer_context.auth_config.user_agent);
    addCurlHeader(&headers, "Accept: %s", request_accept);
    addCurlHeader(&headers, "Client-ID: %s", thread_status[thread_index].dialer_context.auth_config.client_id);
    addCurlHeader(&headers, "Algo-ID: %s", thread_status[thread_index].dialer_context.auth_config.algo_id);
    addCurlHeader(&headers, "CDC-SchoolId: %s", school_id);
    addCurlHeader(&headers, "CDC-Domain: %s", domain);
    addCurlHeader(&headers, "CDC-Area: %s", area);
    response = post(url, data, headers);
    return response;
}

static void getSchoolNetworkSymbol()
{
    if (school_network_symbol[0] != '\0') return;
    const char* wlan_user_ip_tag = strstr(check_location, "wlanuserip");
    if (wlan_user_ip_tag)
    {
        const char* wlan_user_ip_first_start = wlan_user_ip_tag + 11;
        const char* wlan_user_ip_first_end = strchr(wlan_user_ip_first_start, '.');
        if (wlan_user_ip_first_end)
        {
            const char* wlan_user_ip_second_start = wlan_user_ip_first_end + 1;
            const char* wlan_user_ip_second_end = strchr(wlan_user_ip_second_start, '.');
            const size_t len = wlan_user_ip_second_end - wlan_user_ip_first_start;
            memcpy(school_network_symbol, wlan_user_ip_first_start, len);
            school_network_symbol[len] = '\0';
            LOG_DEBUG("获取到校园网 IP 特征: %s", school_network_symbol);
        }
    }
}

static NetworkStatus getLastLocation()
{
    if (thread_last_location[0] != '\0' || thread_status[thread_index].dialer_context.runtime_status.is_authed) return REQUEST_SUCCESS;
    HTTPResponse response = {0};
    struct curl_slist* headers = NULL;
    addCurlHeader(&headers, "User-Agent: %s", thread_status[thread_index].dialer_context.auth_config.user_agent);
    addCurlHeader(&headers, "Accept: %s", request_accept);
    addCurlHeader(&headers, "Client-ID: %s", thread_status[thread_index].dialer_context.auth_config.client_id);
    response = get(generate_url, headers);
    while (response.status == REQUEST_REDIRECT) response = get(latest_location, headers);
    if (last_location[0] == '\0') snprintf(last_location, LOCATION_LENGTH, "%s", latest_location);
    LOG_DEBUG("Last location: %s", last_location);
    const char* location = last_location;
    const char* wlan_user_ip_tag = strstr(location, "wlanuserip");
    if (!wlan_user_ip_tag)
    {
        LOG_ERROR("查找 wlanuserip 标志失败");
        return REQUEST_ERROR;
    }
    const char* wlan_user_ip_start = wlan_user_ip_tag + 11;
    const size_t len = wlan_user_ip_start - location;
    char* modified_location_head = malloc(len + 1);
    if (!modified_location_head)
    {
        LOG_ERROR("分配内存失败");
        return REQUEST_ERROR;
    }
    memcpy(modified_location_head, location, len);
    modified_location_head[len] = '\0';
    const char* wlan_user_ip_end = strchr(wlan_user_ip_start, '&');
    if (!wlan_user_ip_end)
    {
        LOG_ERROR("查找 wlanuserip 结尾失败");
        return REQUEST_ERROR;
    }
    const size_t wlan_user_ip_len = wlan_user_ip_end - wlan_user_ip_start;
    char* wlan_user_ip = malloc(wlan_user_ip_len + 1);
    memcpy(wlan_user_ip, wlan_user_ip_start, wlan_user_ip_len);
    wlan_user_ip[wlan_user_ip_len] = '\0';
    if (strstr(school_connection_status[0].ip, wlan_user_ip) != NULL)
    {
        if (school_connection_status[0].is_used)
        {
            snprintf(thread_status[thread_index].dialer_context.auth_config.client_ip, IP_LENGTH, "%s", school_connection_status[1].ip);
            school_connection_status[1].is_used = true;
        }
        else
        {
            snprintf(thread_status[thread_index].dialer_context.auth_config.client_ip, IP_LENGTH, "%s", school_connection_status[0].ip);
            school_connection_status[0].is_used = true;
        }
    }
    else
    {
        if (school_connection_status[1].is_used)
        {
            snprintf(thread_status[thread_index].dialer_context.auth_config.client_ip, IP_LENGTH, "%s", school_connection_status[0].ip);
            school_connection_status[0].is_used = true;
        }
        else
        {
            snprintf(thread_status[thread_index].dialer_context.auth_config.client_ip, IP_LENGTH, "%s", school_connection_status[1].ip);
            school_connection_status[1].is_used = true;
        }
    }
    char modified_location[LOCATION_LENGTH] = {0};
    strcat(modified_location, modified_location_head);
    strcat(modified_location, thread_status[thread_index].dialer_context.auth_config.client_ip);
    strcat(modified_location, wlan_user_ip_end);
    snprintf(thread_last_location, LOCATION_LENGTH, "%s", modified_location);
    LOG_DEBUG("Thread last location: %s", thread_last_location);
    free(modified_location_head);
    return REQUEST_SUCCESS;
}

NetworkStatus checkNetworkStatus()
{
    HTTPResponse response = {0};
    response = get(generate_url, NULL);
    if (response.status == REQUEST_REDIRECT)
    {
        getSchoolNetworkSymbol();
    }
    return response.status;
}

NetworkStatus checkAuthStatus()
{
    const char PORTAL_START_TAG[] = "<!--//config.campus.js.chinatelecom.com";
    const char PORTAL_END_TAG[] = "//config.campus.js.chinatelecom.com-->";
    HTTPResponse response = {0};
    struct curl_slist *headers = NULL;
    addCurlHeader(&headers, "User-Agent: %s", thread_status[thread_index].dialer_context.auth_config.user_agent);
    addCurlHeader(&headers, "Accept: %s", request_accept);
    addCurlHeader(&headers, "Client-ID: %s", thread_status[thread_index].dialer_context.auth_config.client_id);
    switch (getLastLocation())
    {
    case REQUEST_SUCCESS:
        if (thread_status[thread_index].dialer_context.runtime_status.is_authed) return REQUEST_SUCCESS;
        break;
    default:
        return REQUEST_ERROR;
    }
    response = get(thread_last_location, headers);
    LOG_INFO("School Id: %s", school_id);
    LOG_INFO("Domain: %s", domain);
    LOG_INFO("Area: %s", area);
    if (response.status == REQUEST_ERROR)
    {
        LOG_ERROR("GET 错误");
        return REQUEST_ERROR;
    }
    char* portal_config = extractBetweenTags(response.body_data, PORTAL_START_TAG, PORTAL_END_TAG);
    free(response.body_data);
    if (!portal_config)
    {
        LOG_ERROR("提取门户配置失败");
        return REQUEST_ERROR;
    }
    char* auth_url = XmlParser(portal_config, "auth-url");
    if (!auth_url)
    {
        LOG_ERROR("提取 Auth URL 失败");
        return REQUEST_ERROR;
    }
    char* cleaned_auth_url = cleanCDATA(auth_url);
    free(auth_url);
    if (!cleaned_auth_url)
    {
        LOG_ERROR("清除 Auth URL 失败");
        return REQUEST_ERROR;
    }
    LOG_INFO("Auth URL: %s", cleaned_auth_url);
    snprintf(thread_status[thread_index].dialer_context.auth_config.auth_url, AUTH_URL_LENGTH, "%s", cleaned_auth_url);
    free(cleaned_auth_url);
    char* ticket_url = XmlParser(portal_config, "ticket-url");
    free(portal_config);
    if (!ticket_url)
    {
        LOG_ERROR("提取 Ticket URL 失败");
        return REQUEST_ERROR;
    }
    char* cleaned_ticket_url = cleanCDATA(ticket_url);
    free(ticket_url);
    if (!cleaned_ticket_url)
    {
        LOG_ERROR("清除 Ticket URL CDATA 失败");
        return REQUEST_ERROR;
    }
    LOG_INFO("Ticket URL: %s", cleaned_ticket_url);
    snprintf(thread_status[thread_index].dialer_context.auth_config.ticket_url, TICKET_URL_LENGTH, "%s", cleaned_ticket_url);
    LOG_INFO("Client IP: %s", thread_status[thread_index].dialer_context.auth_config.client_ip);
    char* ac_ip = extractUrlParam(cleaned_ticket_url, "wlanacip");
    free(cleaned_ticket_url);
    if (!ac_ip)
    {
        LOG_ERROR("提取 AC IP 失败");
        return REQUEST_ERROR;
    }
    LOG_INFO("AC IP: %s", ac_ip);
    snprintf(thread_status[thread_index].dialer_context.auth_config.ac_ip, IP_LENGTH, "%s", ac_ip);
    free(ac_ip);
    return REQUEST_AUTHORIZATION;
}