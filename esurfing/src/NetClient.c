#include "utils/PlatformUtils.h"
#include "utils/Logger.h"
#include "NetClient.h"
#include "States.h"

#include <openssl/evp.h>
#include <curl/curl.h>
#include <string.h>
#include <stdio.h>

static const char request_content_type[] = "application/x-www-form-urlencoded";
static const char request_accept[] = "text/html,text/xml,application/xhtml+xml,application/x-javascript,*/*";
static const char generate_url[] = "http://connect.rom.miui.com/generate_204";
__thread char latest_location[LOCATION_LENGTH];
__thread char check_location[LOCATION_LENGTH];
static char school_id[SCHOOL_ID_LENGTH];
static char domain[DOMAIN_LENGTH];
static char area[AREA_LENGTH];

char school_network_symbol[SCHOOL_NETWORK_SYMBOL];

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
        if (school_id[0] == '\0')
        {
            const char* value = header + 9;
            while (*value == ' ') value++;
            const size_t valid_len = strcspn(value, "\r\n");
            snprintf(school_id, SCHOOL_ID_LENGTH, "%.*s", (int)valid_len, value);
        }
    }
    if (real_size >= 7 && strncmp(header, "domain:", 7) == 0 && !domain[0])
    {
        if (domain[0] == '\0')
        {
            const char* value = header + 7;
            while (*value == ' ') value++;
            const size_t valid_len = strcspn(value, "\r\n");
            snprintf(domain, DOMAIN_LENGTH, "%.*s", (int)valid_len, value);
        }
    }
    if (real_size >= 5 && strncmp(header, "area:", 5) == 0 && !area[0])
    {
        if (area[0] == '\0')
        {
            const char* value = header + 5;
            while (*value == ' ') value++;
            const size_t valid_len = strcspn(value, "\r\n");
            snprintf(area, AREA_LENGTH, "%.*s", (int)valid_len, value);
        }
    }
    if (real_size >= 9 && strncasecmp(header, "Location:", 9) == 0)
    {
        const char* value = header + 9;
        while (*value == ' ') value++;
        const size_t valid_len = strcspn(value, "\r\n");
        snprintf(latest_location, LOCATION_LENGTH, "%.*s", (int)valid_len, value);
        if (check_location[0] == '\0')
        {
            snprintf(check_location, LOCATION_LENGTH, "%.*s", (int)valid_len, value);
            LOG_DEBUG("获取到 Check Location: %s", check_location);
        }
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
    curl_easy_setopt(*curl, CURLOPT_INTERFACE, prog_status[prog_index].login_config.ip);
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
    curl_easy_setopt(*curl, CURLOPT_INTERFACE, prog_status[prog_index].login_config.ip);
    return CURL_INIT_SUCCESS;
}

static void addCurlHeader(struct curl_slist** headers, const char* format, ...)
{
    va_list local_args;
    char header_buffer[128];
    va_start(local_args, format);
    vsnprintf(header_buffer, sizeof(header_buffer), format, local_args);
    va_end(local_args);
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
        LOG_ERROR("HTTP 请求错误: %s ,错误码: %d, GET URL: %s", error_msg, res, url);
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
    addCurlHeader(&headers, "User-Agent: %s", prog_status[prog_index].auth_config.user_agent);
    addCurlHeader(&headers, "Accept: %s", request_accept);
    addCurlHeader(&headers, "Client-ID: %s", prog_status[prog_index].auth_config.client_id);
    addCurlHeader(&headers, "Algo-ID: %s", prog_status[prog_index].auth_config.algo_id);
    addCurlHeader(&headers, "CDC-SchoolId: %s", school_id);
    addCurlHeader(&headers, "CDC-Domain: %s", domain);
    addCurlHeader(&headers, "CDC-Area: %s", area);
    response = post(url, data, headers);
    return response;
}

static void getSchoolNetworkSymbol()
{
    if (school_network_symbol[0] != '\0')
    {
        LOG_VERBOSE("已有特征值: %s", school_network_symbol);
        return;
    }
    const char* wlan_user_ip_tag = strstr(check_location, "wlanuserip");
    if (!wlan_user_ip_tag)
    {
        LOG_WARN("未找到校园网特征, 检查 Location: %s", check_location);
        return;
    }
    const char* wlan_user_ip_first_start = wlan_user_ip_tag + 11;
    const char* wlan_user_ip_first_end = strchr(wlan_user_ip_first_start, '.');
    if (!wlan_user_ip_first_end)
    {
        LOG_ERROR("未找到尾部");
        return;
    }
    const char* wlan_user_ip_second_start = wlan_user_ip_first_end + 1;
    const char* wlan_user_ip_second_end = strchr(wlan_user_ip_second_start, '.');
    const size_t len = wlan_user_ip_second_end - wlan_user_ip_first_start;
    memcpy(school_network_symbol, wlan_user_ip_first_start, len);
    school_network_symbol[len] = '\0';
    LOG_DEBUG("获取到校园网 IP 特征: %s", school_network_symbol);
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
    addCurlHeader(&headers, "User-Agent: %s", prog_status[prog_index].auth_config.user_agent);
    addCurlHeader(&headers, "Accept: %s", request_accept);
    addCurlHeader(&headers, "Client-ID: %s", prog_status[prog_index].auth_config.client_id);
    response.status = checkNetworkStatus();
    while (response.status == REQUEST_REDIRECT) response = get(latest_location, headers);
    if (response.status == REQUEST_SUCCESS || response.status == REQUEST_WARNING) return response.status;
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
    snprintf(prog_status[prog_index].auth_config.auth_url, AUTH_URL_LENGTH, "%s", cleaned_auth_url);
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
    snprintf(prog_status[prog_index].auth_config.ticket_url, TICKET_URL_LENGTH, "%s", cleaned_ticket_url);
    LOG_INFO("Client IP: %s", prog_status[prog_index].auth_config.client_ip);
    char* ac_ip = extractUrlParam(cleaned_ticket_url, "wlanacip");
    free(cleaned_ticket_url);
    if (!ac_ip)
    {
        LOG_ERROR("提取 AC IP 失败");
        return REQUEST_ERROR;
    }
    LOG_INFO("AC IP: %s", ac_ip);
    snprintf(prog_status[prog_index].auth_config.ac_ip, IP_LENGTH, "%s", ac_ip);
    free(ac_ip);
    return REQUEST_AUTHORIZATION;
}