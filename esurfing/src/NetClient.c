#include <openssl/evp.h>
#include <curl/curl.h>
#include <string.h>
#include <stdio.h>

#include "headFiles/utils/PlatformUtils.h"
#include "headFiles/utils/Logger.h"
#include "headFiles/NetClient.h"
#include "headFiles/States.h"

static const char request_accept[] = "text/html,text/xml,application/xhtml+xml,application/x-javascript,*/*";
static const char generate_url[] = "http://www.gstatic.com/generate_204";
static char base_url[LOCATION_LENGTH] = "";
static char last_location[LOCATION_LENGTH] = "";
static size_t location_size = 0;

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
    if (thread_status[thread_index].dialer_context.auth_config.school_id[0]
        && thread_status[thread_index].dialer_context.auth_config.domain[0]
        && thread_status[thread_index].dialer_context.auth_config.area[0])
        return real_size;
    if (real_size >= 9 && strncmp(header, "schoolid:", 9) == 0)
    {
        const char* value = header + 9;
        while (*value == ' ') value++;
        const size_t valid_len = strcspn(value, "\r\n");
        snprintf(thread_status[thread_index].dialer_context.auth_config.school_id, SCHOOL_ID_LENGTH, "%.*s", (int)valid_len, value);
    }
    if (real_size >= 7 && strncmp(header, "domain:", 7) == 0)
    {
        const char* value = header + 7;
        while (*value == ' ') value++;
        const size_t valid_len = strcspn(value, "\r\n");
        snprintf(thread_status[thread_index].dialer_context.auth_config.domain, DOMAIN_LENGTH, "%.*s", (int)valid_len, value);
    }
    if (real_size >= 5 && strncmp(header, "area:", 5) == 0)
    {
        const char* value = header + 5;
        while (*value == ' ') value++;
        const size_t valid_len = strcspn(value, "\r\n");
        snprintf(thread_status[thread_index].dialer_context.auth_config.area, AREA_LENGTH, "%.*s", (int)valid_len, value);
    }
    if (real_size >= 9 && strncasecmp(header, "Location:", 9) == 0)
    {
        const char* value = header + 9;
        while (*value == ' ') value++;
        const size_t valid_len = strcspn(value, "\r\n");
        snprintf(last_location, LOCATION_LENGTH, "%.*s", (int)valid_len, value);
        if (strstr(last_location, "http://") != NULL || strstr(last_location, "https://") != NULL)
        {
            const char* start = last_location;
            if (strstr(last_location, "http://") != NULL) start += 7;
            else if (strstr(last_location, "https://") != NULL) start += 8;
            const char* end = strchr(start, '/');
            const size_t len = end - last_location;
            memcpy(base_url, last_location, len);
            base_url[len] = '\0';
        }
        else
        {
            char* tmp = strdup(last_location);
            snprintf(last_location, LOCATION_LENGTH, "%s%s", base_url, tmp);
            free(tmp);
        }
        location_size = strlen(last_location);
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
        response.status = REQUEST_ERROR;
        return response;
    }
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response.response_code);
    curl_easy_cleanup(curl);
    if (response.response_code == 302)
    {
        LOG_DEBUG("重定向至: %s", last_location);
        response.status = REQUEST_REDIRECT;
        return response;
    }
    if (response.response_code < 200 || response.response_code >= 300)
    {
        LOG_ERROR("HTTP 响应错误, 响应码: %d", response.response_code);
        response.status = REQUEST_ERROR;
        return response;
    }
    response.status = REQUEST_SUCCESS;
    return response;
}

HTTPResponse sessionPost(const char* url, const char* data)
{
    struct curl_slist* headers;
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
    addCurlHeader(&headers, "Content-Type: application/x-www-form-urlencoded");
    addCurlHeader(&headers, "User-Agent: %s", thread_status[thread_index].dialer_context.auth_config.user_agent);
    addCurlHeader(&headers, "Accept: %s", request_accept);
    addCurlHeader(&headers, "Client-ID: %s", thread_status[thread_index].dialer_context.auth_config.client_id);
    addCurlHeader(&headers, "Algo-ID: %s", thread_status[thread_index].dialer_context.auth_config.algo_id);
    addCurlHeader(&headers, "CDC-SchoolId: %s", thread_status[thread_index].dialer_context.auth_config.school_id);
    addCurlHeader(&headers, "CDC-Domain: %s", thread_status[thread_index].dialer_context.auth_config.domain);
    addCurlHeader(&headers, "CDC-Area: %s", thread_status[thread_index].dialer_context.auth_config.area);
    response = post(url, data, headers);
    return response;
}

NetworkStatus checkNetworkStatus()
{
    CURL* curl = curl_easy_init();
    HTTPResponse response = {0};
    const char url[] = "http://www.baidu.com";
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);
    const CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        const char* error_msg = curl_easy_strerror(res);
        LOG_ERROR("HTTP 请求错误: %s (错误码: %d)", error_msg, res);
        curl_easy_cleanup(curl);
        response.status = REQUEST_ERROR;
        return response.status;
    }
    long response_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    curl_easy_cleanup(curl);
    if (response_code < 200 || response_code >= 300) response.status = REQUEST_ERROR;
    else response.status = REQUEST_SUCCESS;
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
    response = get(generate_url, headers);
    while (response.status == REQUEST_REDIRECT) get(last_location, headers);
    if (response.status == REQUEST_ERROR)
    {
        LOG_ERROR("GET 错误");
        return REQUEST_ERROR;
    }
    if (response.response_code == 204) return REQUEST_SUCCESS;
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
    char* ticket_url = XmlParser(portal_config, "ticket-url");
    free(portal_config);
    if (!ticket_url)
    {
        LOG_ERROR("提取 Ticket URL 失败");
        return REQUEST_ERROR;
    }
    char* cleaned_auth_url = cleanCDATA(auth_url);
    free(auth_url);
    if (!cleaned_auth_url)
    {
        LOG_ERROR("清除 Auth URL 失败");
        return REQUEST_ERROR;
    }
    snprintf(thread_status[thread_index].dialer_context.auth_config.auth_url, AUTH_URL_LENGTH, "%s", cleaned_auth_url);
    free(cleaned_auth_url);
    char* new_ticket_url = cleanCDATA(ticket_url);
    free(ticket_url);
    if (!new_ticket_url)
    {
        LOG_ERROR("清除 Ticket URL CDATA 失败");
        return REQUEST_ERROR;
    }
    snprintf(thread_status[thread_index].dialer_context.auth_config.ticket_url, TICKET_URL_LENGTH, "%s", new_ticket_url);
    char* client_ip = extractUrlParam(new_ticket_url, "wlanuserip");
    if (!client_ip)
    {
        LOG_ERROR("提取 Client IP 失败");
        return REQUEST_ERROR;
    }
    snprintf(thread_status[thread_index].dialer_context.auth_config.client_ip, CLIENT_IP_LENGTH, "%s", client_ip);
    char* ac_ip = extractUrlParam(new_ticket_url, "wlanacip");
    free(new_ticket_url);
    if (!ac_ip)
    {
        LOG_ERROR("提取 AC IP 失败");
        return REQUEST_ERROR;
    }
    snprintf(thread_status[thread_index].dialer_context.auth_config.ac_ip, AC_IP_LENGTH, "%s", ac_ip);
    return REQUEST_AUTHORIZATION;
}