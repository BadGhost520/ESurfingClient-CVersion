#include <openssl/evp.h>
#include <curl/curl.h>
#include <string.h>
#include <stdio.h>

#include "headFiles/utils/PlatformUtils.h"
#include "headFiles/utils/Logger.h"
#include "headFiles/Constants.h"
#include "headFiles/NetClient.h"

#include "headFiles/States.h"

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

static size_t writeResponseCallback(const void *contents, const size_t size, const size_t nmemb, HTTPResponse *response)
{
    const size_t realSize = size * nmemb;
    char *ptr = realloc(response->data, response->dataSize + realSize + 1);
    if (!ptr) return 0;
    response->data = ptr;
    memcpy(&response->data[response->dataSize], contents, realSize);
    response->dataSize += realSize;
    response->data[response->dataSize] = 0;
    return realSize;
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

void freeResult(HTTPResponse* result)
{
    if (!result)
    {
        LOG_DEBUG("返回值为空");
        return;
    }
    if (result->data)
    {
        free(result->data);
        result->data = NULL;
    }
    free(result);
}

HTTPResponse* simPost(const char* url, const char* data)
{
    HTTPResponse* result = malloc(sizeof(HTTPResponse));
    if (!result)
    {
        LOG_ERROR("分配内存失败");
        return NULL;
    }
    HTTPResponse response = {0};
    struct curl_slist* headers = NULL;
    char headerBuffer[512];
    result->data = NULL;
    result->dataSize = 0;
    CURL* curl = curl_easy_init();
    if (!curl)
    {
        LOG_ERROR("初始化 Curl 错误");
        result->status = REQUEST_ERROR;
        return result;
    }
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
    headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
    snprintf(headerBuffer, sizeof(headerBuffer), "User-Agent: %s", USER_AGENT);
    headers = curl_slist_append(headers, headerBuffer);
    snprintf(headerBuffer, sizeof(headerBuffer), "Accept: %s", REQUEST_ACCEPT);
    headers = curl_slist_append(headers, headerBuffer);
    char* MD5Hash = calculateMD5(data);
    if (!MD5Hash)
    {
        LOG_ERROR("计算 MD5 失败");
        result->status = REQUEST_ERROR;
        return result;
    }
    snprintf(headerBuffer, sizeof(headerBuffer), "CDC-Checksum: %s", MD5Hash);
    headers = curl_slist_append(headers, headerBuffer);
    free(MD5Hash);
    if (!dialer_adapter.auth_config.client_id)
    {
        LOG_ERROR("Client ID 为空");
        result->status = REQUEST_ERROR;
        return result;
    }
    if (!dialer_adapter.auth_config.algo_id)
    {
        LOG_ERROR("Algo ID 为空");
        result->status = REQUEST_ERROR;
        return result;
    }
    if (!dialer_adapter.auth_config.school_id)
    {
        LOG_ERROR("School ID 为空");
        result->status = REQUEST_ERROR;
        return result;
    }
    if (!dialer_adapter.auth_config.domain)
    {
        LOG_ERROR("Domain 为空");
        result->status = REQUEST_ERROR;
        return result;
    }
    if (!dialer_adapter.auth_config.area)
    {
        LOG_ERROR("Area 为空");
        result->status = REQUEST_ERROR;
        return result;
    }
    snprintf(headerBuffer, sizeof(headerBuffer), "Client-ID: %s", dialer_adapter.auth_config.client_id);
    headers = curl_slist_append(headers, headerBuffer);
    snprintf(headerBuffer, sizeof(headerBuffer), "Algo-ID: %s", dialer_adapter.auth_config.algo_id);
    headers = curl_slist_append(headers, headerBuffer);
    snprintf(headerBuffer, sizeof(headerBuffer), "CDC-SchoolId: %s", dialer_adapter.auth_config.school_id);
    headers = curl_slist_append(headers, headerBuffer);
    snprintf(headerBuffer, sizeof(headerBuffer), "CDC-Domain: %s", dialer_adapter.auth_config.domain);
    headers = curl_slist_append(headers, headerBuffer);
    snprintf(headerBuffer, sizeof(headerBuffer), "CDC-Area: %s", dialer_adapter.auth_config.area);
    headers = curl_slist_append(headers, headerBuffer);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeResponseCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    const CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        result->status = REQUEST_ERROR;
        LOG_ERROR("网络错误，原因: %s",curl_easy_strerror(res));
        return result;
    }
    result->data = response.data;
    result->dataSize = response.dataSize;
    free(response.data);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    result->status = REQUEST_SUCCESS;
    return result;
}

NetworkStatus checkNetworkStatus()
{
    const char PORTAL_START_TAG[] = "<!--//config.campus.js.chinatelecom.com";
    const char PORTAL_END_TAG[] = "//config.campus.js.chinatelecom.com-->";
    HTTPResponse response_data = {0};
    CURL* curl = curl_easy_init();
    if (!curl)
    {
        LOG_ERROR("初始化 Curl 错误");
        return INIT_ERROR;
    }
    curl_easy_setopt(curl, CURLOPT_URL, CAPTIVE_URL);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);
    struct curl_slist *headers = NULL;
    char header_buffer[256];
    if (!USER_AGENT)
    {
        LOG_ERROR("User Agent 不存在");
        return INIT_ERROR;
    }
    if (!dialer_adapter.auth_config.client_id)
    {
        LOG_ERROR("Client ID 不存在");
        return INIT_ERROR;
    }
    snprintf(header_buffer, sizeof(header_buffer), "User-Agent: %s", USER_AGENT);
    headers = curl_slist_append(headers, header_buffer);
    snprintf(header_buffer, sizeof(header_buffer), "Accept: %s", REQUEST_ACCEPT);
    headers = curl_slist_append(headers, header_buffer);
    snprintf(header_buffer, sizeof(header_buffer), "Client-ID: %s", dialer_adapter.auth_config.client_id);
    headers = curl_slist_append(headers, header_buffer);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeResponseCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
    const CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        const char* error_msg = curl_easy_strerror(res);
        LOG_ERROR("HTTP 请求错误: %s, 错误码: %d", error_msg, res);
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        free(response_data.data);
        return REQUEST_ERROR;
    }
    long response_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    if (response_code == 204)
    {
        free(response_data.data);
        return REQUEST_SUCCESS;
    }
    if (response_code != 200 && response_code != 302)
    {
        LOG_ERROR("HTTP 响应错误, 响应码: %d", response_code);
        free(response_data.data);
        return REQUEST_ERROR;
    }
    char* portal_config = extractBetweenTags(response_data.data, PORTAL_START_TAG, PORTAL_END_TAG);
    free(response_data.data);
    if (!portal_config)
    {
        LOG_ERROR("提取门户配置失败");
        return REQUEST_ERROR;
    }
    char* auth_url = XmlParser(portal_config, "auth-url");
    char* ticket_url = XmlParser(portal_config, "ticket-url");
    free(portal_config);
    if (!auth_url || !ticket_url)
    {
        if (!auth_url) LOG_ERROR("提取 Auth URL 失败");
        else LOG_ERROR("提取 Ticket URL 失败");
        return REQUEST_ERROR;
    }
    char* new_auth_url = cleanCDATA(auth_url);
    char* new_ticket_url = cleanCDATA(ticket_url);
    free(auth_url);
    free(ticket_url);
    if (!new_auth_url || !new_ticket_url)
    {
        if (!new_auth_url) LOG_ERROR("清除 Auth URL 失败");
        else LOG_ERROR("清除 Ticket URL CDATA 失败");
        return REQUEST_ERROR;
    }
    dialer_adapter.auth_config.auth_url = new_auth_url;
    dialer_adapter.auth_config.ticket_url = new_ticket_url;
    char* user_ip = extractUrlParam(new_ticket_url, "wlanuserip");
    char* ac_ip = extractUrlParam(new_ticket_url, "wlanacip");
    if (!user_ip || !ac_ip)
    {
        if (!user_ip) LOG_ERROR("提取 User IP 失败");
        else LOG_ERROR("提取 AC IP 失败");
        return REQUEST_ERROR;
    }
    return REQUEST_AUTHORIZATION;
}

NetworkStatus simGet(char* url)
{
    CURL* curl = curl_easy_init();
    if (!curl)
    {
        LOG_ERROR("初始化 Curl 错误");
        return INIT_ERROR;
    }
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    const CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        const char* error_msg = curl_easy_strerror(res);
        LOG_ERROR("HTTP 请求错误: %s (错误码: %d)", error_msg, res);
        curl_easy_cleanup(curl);
        return REQUEST_ERROR;
    }
    long response_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    curl_easy_cleanup(curl);
    if (response_code < 200 || response_code >= 300)
    {
        LOG_ERROR("HTTP 响应错误, 响应码: %d", response_code);
        return REQUEST_ERROR;
    }
    return REQUEST_SUCCESS;
}