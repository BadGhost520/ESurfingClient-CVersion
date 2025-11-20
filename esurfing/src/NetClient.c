//
// Created by bad_g on 2025/9/23.
//
#include <stdio.h>
#include <string.h>
#include <openssl/evp.h>
#include <curl/curl.h>

#include "headFiles/NetClient.h"
#include "headFiles/Constants.h"
#include "headFiles/States.h"

size_t writeResponseCallback(const void *contents, const size_t size, const size_t nmemb, HTTPResponse *response)
{
    const size_t realSize = size * nmemb;
    char *ptr = realloc(response->memory, response->size + realSize + 1);
    if (!ptr) return 0;
    response->memory = ptr;
    memcpy(&response->memory[response->size], contents, realSize);
    response->size += realSize;
    response->memory[response->size] = 0;
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

int ensureCurlInitialized()
{
    int initialized = 0;
    if (!initialized)
    {
        if (curl_global_init(CURL_GLOBAL_DEFAULT) != 0) {
            return -1;
        }
        initialized = 1;
        atexit(curl_global_cleanup);
    }
    return 0;
}

void freeNetResult(NetResult* result)
{
    if (result)
    {
        if (result->data)
        {
            free(result->data);
        }
        if (result->errorMessage)
        {
            free(result->errorMessage);
        }
        free(result);
    }
}

NetResult* postRequest(const char* url, const char* data, ExtraHeaders* extraHeaders)
{
    NetResult* result = malloc(sizeof(NetResult));
    HTTPResponse response = {0};
    struct curl_slist* headers = NULL;
    char headerBuffer[512];
    if (!result) return NULL;
    if (ensureCurlInitialized() != 0)
    {
        result->type = NET_RESULT_ERROR;
        result->data = NULL;
        result->errorMessage = strdup("初始化 Curl 库失败");
        result->statusCode = 0;
        return result;
    }
    result->type = NET_RESULT_ERROR;
    result->data = NULL;
    result->dataSize = 0;
    result->errorMessage = NULL;
    result->statusCode = 0;
    CURL* curl = curl_easy_init();
    if (!curl)
    {
        result->errorMessage = strdup("初始化 Curl 失败");
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
    if (strlen(clientId) > 0)
    {
        snprintf(headerBuffer, sizeof(headerBuffer), "Client-ID: %s", clientId);
        headers = curl_slist_append(headers, headerBuffer);
    }
    if (strlen(algoId) > 0)
    {
        snprintf(headerBuffer, sizeof(headerBuffer), "Algo-ID: %s", algoId);
        headers = curl_slist_append(headers, headerBuffer);
    }
    if (extraHeaders)
    {
        for (int i = 0; i < extraHeaders->count; i++)
        {
            snprintf(headerBuffer, sizeof(headerBuffer), "%s: %s", extraHeaders->headers[i].key, extraHeaders->headers[i].value);
            headers = curl_slist_append(headers, headerBuffer);
        }
    }
    if (schoolId != NULL)
    {
        snprintf(headerBuffer, sizeof(headerBuffer), "CDC-SchoolId: %s", schoolId);
        headers = curl_slist_append(headers, headerBuffer);
    }
    if (domain != NULL)
    {
        snprintf(headerBuffer, sizeof(headerBuffer), "CDC-Domain: %s", domain);
        headers = curl_slist_append(headers, headerBuffer);
    }
    if (area != NULL)
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
        result->type = NET_RESULT_ERROR;
        result->errorMessage = strdup(curl_easy_strerror(res));
    }
    else
    {
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        result->statusCode = (int)response_code;
        result->type = NET_RESULT_SUCCESS;
        result->data = response.memory;
        result->dataSize = response.size;
        response.memory = NULL;
    }
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    if (response.memory)
    {
        free(response.memory);
    }
    return result;
}

NetResult* simPost(const char* url, const char* data)
{
    return postRequest(url, data, NULL);
}