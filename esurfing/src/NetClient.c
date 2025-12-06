//
// Created by bad_g on 2025/9/23.
//
#include <stdio.h>
#include <string.h>
#include <openssl/evp.h>
#include <curl/curl.h>

#include "headFiles/States.h"
#include "headFiles/NetClient.h"
#include "headFiles/Constants.h"
#include "headFiles/utils/Logger.h"
#include "headFiles/utils/PlatformUtils.h"

char* extractBetweenTags(const char* text, const char* start_tag, const char* end_tag)
{
    char* start = strstr(text, start_tag);
    if (!start) return NULL;
    start += strlen(start_tag);
    char* end = strstr(start, end_tag);
    if (!end) return NULL;
    const size_t len = end - start;
    char* result = malloc(len + 1);
    if (!result) return NULL;
    strncpy(result, start, len);
    result[len] = '\0';
    return result;
}

char* extractUrlParameter(const char* url, const char* param_name)
{
    char search_pattern[256];
    snprintf(search_pattern, sizeof(search_pattern), "%s=", param_name);
    char* param_start = strstr(url, search_pattern);
    if (!param_start) return NULL;
    param_start += strlen(search_pattern);
    char* param_end = strchr(param_start, '&');
    if (!param_end) param_end = param_start + strlen(param_start);
    const size_t len = param_end - param_start;
    char* result = malloc(len + 1);
    if (!result) return NULL;
    strncpy(result, param_start, len);
    result[len] = '\0';
    return result;
}

size_t writeResponseCallback(const void *contents, const size_t size, const size_t nmemb, HTTPResponse *response)
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

char* calculateMD5(const char* data)
{
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digestLen;
    char* MD5String = malloc(33);
    if (MD5String == NULL) return NULL;
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (mdctx == NULL)
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
    for (unsigned int i = 0; i < digestLen; i++)
    {
        sprintf(&MD5String[i*2], "%02x", (unsigned int)digest[i]);
    }
    return MD5String;
}

void freeResult(HTTPResponse* result)
{
    if (result)
    {
        if (result->data) free(result->data);
        free(result);
    }
}

HTTPResponse* simPost(const char* url, const char* data)
{
    HTTPResponse* result = malloc(sizeof(HTTPResponse));
    HTTPResponse response = {0};
    struct curl_slist* headers = NULL;
    char headerBuffer[512];
    if (!result) return NULL;
    result->data = NULL;
    result->dataSize = 0;
    CURL* curl = curl_easy_init();
    if (!curl)
    {
        result->status = RequestError;
        LOG_ERROR("初始化 Curl 失败");
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
    if (MD5Hash)
    {
        snprintf(headerBuffer, sizeof(headerBuffer), "CDC-Checksum: %s", MD5Hash);
        headers = curl_slist_append(headers, headerBuffer);
        free(MD5Hash);
    }
    if (clientId[0])
    {
        snprintf(headerBuffer, sizeof(headerBuffer), "Client-ID: %s", clientId);
        headers = curl_slist_append(headers, headerBuffer);
    }
    if (algoId[0])
    {
        snprintf(headerBuffer, sizeof(headerBuffer), "Algo-ID: %s", algoId);
        headers = curl_slist_append(headers, headerBuffer);
    }
    if (schoolId && schoolId[0])
    {
        snprintf(headerBuffer, sizeof(headerBuffer), "CDC-SchoolId: %s", schoolId);
        headers = curl_slist_append(headers, headerBuffer);
    }
    if (domain && domain[0])
    {
        snprintf(headerBuffer, sizeof(headerBuffer), "CDC-Domain: %s", domain);
        headers = curl_slist_append(headers, headerBuffer);
    }
    if (area && area[0])
    {
        snprintf(headerBuffer, sizeof(headerBuffer), "CDC-Area: %s", area);
        headers = curl_slist_append(headers, headerBuffer);
    }
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeResponseCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    const CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        result->status = RequestError;
        LOG_ERROR("网络错误，原因: %s",curl_easy_strerror(res));
    }
    else
    {
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        result->data = response.data;
        result->dataSize = response.dataSize;
        response.data = NULL;
    }
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    if (response.data)
    {
        free(response.data);
    }
    result->status = RequestSuccess;
    return result;
}

NetworkStatus checkNetworkStatus()
{
    long response_code = 0;
    HTTPResponse response_data = {0};
    CURL* curl = curl_easy_init();
    if (!curl)
    {
        LOG_ERROR("初始化 Curl 错误");
        return InitError;
    }
    curl_easy_setopt(curl, CURLOPT_URL, CAPTIVE_URL);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);
    struct curl_slist *headers = NULL;
    char header_buffer[256];
    if (USER_AGENT && USER_AGENT[0])
    {
        snprintf(header_buffer, sizeof(header_buffer), "User-Agent: %s", USER_AGENT);
        headers = curl_slist_append(headers, header_buffer);
    }
    else
    {
        LOG_ERROR("User Agent 不存在");
        return InitError;
    }
    snprintf(header_buffer, sizeof(header_buffer), "Accept: %s", REQUEST_ACCEPT);
    headers = curl_slist_append(headers, header_buffer);
    if (clientId && clientId[0])
    {
        snprintf(header_buffer, sizeof(header_buffer), "Client-ID: %s", clientId);
        headers = curl_slist_append(headers, header_buffer);
    }
    else
    {
        LOG_ERROR("Client ID 不存在");
        return InitError;
    }
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeResponseCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
    const CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        if (response_data.data) free(response_data.data);
        const char* error_msg = curl_easy_strerror(res);
        LOG_ERROR("HTTP 请求错误: %s (错误码: %d)", error_msg, res);
        return RequestError;
    }
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    if (response_code == 204)
    {
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        if (response_data.data) free(response_data.data);
        return RequestSuccess;
    }
    if (response_code != 200 && response_code != 302)
    {
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        if (response_data.data) free(response_data.data);
        LOG_ERROR("HTTP 响应错误, 响应码: %d", response_code);
        return RequestError;
    }
    if (response_data.data && response_data.dataSize > 0)
    {
        char* portal_config = extractBetweenTags(response_data.data, PORTAL_START_TAG, PORTAL_END_TAG);
        if (portal_config && portal_config[0])
        {
            char* auth_url_raw = XmlParser(portal_config, "auth-url");
            char* ticket_url_raw = XmlParser(portal_config, "ticket-url");
            char* auth_url = cleanCDATA(auth_url_raw);
            char* ticket_url = cleanCDATA(ticket_url_raw);
            if (auth_url_raw) free(auth_url_raw);
            if (ticket_url_raw) free(ticket_url_raw);
            if (auth_url && ticket_url && auth_url[0] && ticket_url[0])
            {
                if (authUrl) free(authUrl);
                authUrl = strdup(auth_url);
                if (ticketUrl) free(ticketUrl);
                ticketUrl = strdup(ticket_url);
                char* user_ip = extractUrlParameter(ticket_url, "wlanuserip");
                char* ac_ip = extractUrlParameter(ticket_url, "wlanacip");
                if (user_ip && ac_ip)
                {
                    if (userIp) free(userIp);
                    userIp = strdup(user_ip);
                    if (acIp) free(acIp);
                    acIp = strdup(ac_ip);
                    free(user_ip);
                    free(ac_ip);
                    free(auth_url);
                    free(ticket_url);
                    free(portal_config);
                    curl_easy_cleanup(curl);
                    curl_slist_free_all(headers);
                    free(response_data.data);
                    return RequestAuthorization;
                }
                if (user_ip) free(user_ip);
                if (ac_ip) free(ac_ip);
            }
            if (auth_url) free(auth_url);
            if (ticket_url) free(ticket_url);
            free(portal_config);
        }
    }
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    if (response_data.data) free(response_data.data);
    return RequestSuccess;
}