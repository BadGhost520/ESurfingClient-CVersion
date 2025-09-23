//
// Created by bad_g on 2025/9/23.
//
#include "headFiles/NetClient.h"
#include "headFiles/Constants.h"
#include "headFiles/States.h"

#include <stdio.h>
#include <string.h>
#include <openssl/evp.h>
#include <curl/curl.h>

// libcurl响应回调函数
size_t write_response_callback(void* contents, size_t size, size_t nmemb, ResponseData* response) {
    size_t real_size = size * nmemb;
    char* ptr = realloc(response->memory, response->size + real_size + 1);

    if (ptr == NULL) {
        printf("Not enough memory (realloc returned NULL)\n");
        return 0;
    }

    response->memory = ptr;
    memcpy(&(response->memory[response->size]), contents, real_size);
    response->size += real_size;
    response->memory[response->size] = 0;

    return real_size;
}

// MD5计算函数 - 使用OpenSSL 3.0+兼容的EVP API
char* calculate_md5(const char* data) {
    EVP_MD_CTX* mdctx;
    const EVP_MD* md;
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len;
    char* md5_string = malloc(33); // 32字符 + null终止符

    if (md5_string == NULL) return NULL;

    // 初始化EVP上下文
    mdctx = EVP_MD_CTX_new();
    if (mdctx == NULL) {
        free(md5_string);
        return NULL;
    }

    // 获取MD5算法
    md = EVP_md5();

    // 初始化摘要操作
    if (EVP_DigestInit_ex(mdctx, md, NULL) != 1) {
        EVP_MD_CTX_free(mdctx);
        free(md5_string);
        return NULL;
    }

    // 更新摘要数据
    if (EVP_DigestUpdate(mdctx, data, strlen(data)) != 1) {
        EVP_MD_CTX_free(mdctx);
        free(md5_string);
        return NULL;
    }

    // 完成摘要计算
    if (EVP_DigestFinal_ex(mdctx, digest, &digest_len) != 1) {
        EVP_MD_CTX_free(mdctx);
        free(md5_string);
        return NULL;
    }

    // 清理上下文
    EVP_MD_CTX_free(mdctx);

    // 转换为十六进制字符串
    for (unsigned int i = 0; i < digest_len; i++) {
        sprintf(&md5_string[i*2], "%02x", (unsigned int)digest[i]);
    }

    return md5_string;
}

// 初始化额外请求头
void init_extra_headers(ExtraHeaders* headers) {
    if (headers) {
        headers->count = 0;
        memset(headers->headers, 0, sizeof(headers->headers));
    }
}

// 添加额外请求头
void add_extra_header(ExtraHeaders* headers, const char* key, const char* value) {
    if (headers && headers->count < MAX_HEADERS_COUNT && key && value) {
        strncpy(headers->headers[headers->count].key, key, MAX_HEADER_LENGTH - 1);
        strncpy(headers->headers[headers->count].value, value, MAX_HEADER_LENGTH - 1);
        headers->count++;
    }
}

// 自动初始化和清理的内部函数
static int ensure_curl_initialized(void) {
    static int initialized = 0;
    if (!initialized) {
        if (curl_global_init(CURL_GLOBAL_DEFAULT) != 0) {
            return -1;
        }
        initialized = 1;
        // 注册程序退出时的清理函数
        atexit(curl_global_cleanup);
    }
    return 0;
}

// 初始化POST客户端 (保留兼容性，但现在是可选的)
int init_post_client(void) {
    return ensure_curl_initialized();
}

// 清理POST客户端 (保留兼容性，但现在是可选的)
void cleanup_post_client(void) {
    // 由于使用了atexit，这个函数现在是可选的
    // 保留是为了向后兼容
}

// 释放NetResult内存
void free_net_result(NetResult* result) {
    if (result) {
        if (result->data) {
            free(result->data);
        }
        if (result->error_message) {
            free(result->error_message);
        }
        free(result);
    }
}

// 主要的POST请求函数 - 对应Kotlin的post函数
NetResult* post_request(const char* url, const char* data, ExtraHeaders* extra_headers) {
    CURL* curl;
    CURLcode res;
    NetResult* result = malloc(sizeof(NetResult));
    ResponseData response = {0};
    struct curl_slist* headers = NULL;
    char header_buffer[512];

    if (!result) return NULL;

    // 自动初始化curl (如果还没有初始化)
    if (ensure_curl_initialized() != 0) {
        result->type = NET_RESULT_ERROR;
        result->data = NULL;
        result->error_message = strdup("Failed to initialize CURL library");
        result->status_code = 0;
        return result;
    }

    // 初始化结果结构体
    result->type = NET_RESULT_ERROR;
    result->data = NULL;
    result->error_message = NULL;
    result->status_code = 0;

    curl = curl_easy_init();
    if (!curl) {
        result->error_message = strdup("Failed to initialize CURL");
        return result;
    }

    // 设置URL
    curl_easy_setopt(curl, CURLOPT_URL, url);

    // 设置POST数据
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);

    // 设置Content-Type
    headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");

    // 设置基本请求头 - 对应Kotlin代码中的基本头信息
    snprintf(header_buffer, sizeof(header_buffer), "User-Agent: %s", USER_AGENT);
    headers = curl_slist_append(headers, header_buffer);

    snprintf(header_buffer, sizeof(header_buffer), "Accept: %s", REQUEST_ACCEPT);
    headers = curl_slist_append(headers, header_buffer);

    // 计算并添加CDC-Checksum头 - 对应DigestUtils.md5Hex(data)
    char* md5_hash = calculate_md5(data);
    if (md5_hash) {
        snprintf(header_buffer, sizeof(header_buffer), "CDC-Checksum: %s", md5_hash);
        headers = curl_slist_append(headers, header_buffer);
        free(md5_hash);
    }

    // 添加Client-ID和Algo-ID头
    if (strlen(clientId) > 0) {
        snprintf(header_buffer, sizeof(header_buffer), "Client-ID: %s", clientId);
        headers = curl_slist_append(headers, header_buffer);
    }

    if (strlen(algoId) > 0) {
        snprintf(header_buffer, sizeof(header_buffer), "Algo-ID: %s", algoId);
        headers = curl_slist_append(headers, header_buffer);
    }

    // 添加额外的请求头 - 对应extraHeaders.forEach
    if (extra_headers) {
        for (int i = 0; i < extra_headers->count; i++) {
            snprintf(header_buffer, sizeof(header_buffer), "%s: %s",
                    extra_headers->headers[i].key, extra_headers->headers[i].value);
            headers = curl_slist_append(headers, header_buffer);
        }
    }
    // 添加条件性请求头 - 对应Kotlin中的条件判断
    if (schoolId != NULL) {
        snprintf(header_buffer, sizeof(header_buffer), "CDC-SchoolId: %s", schoolId);
        headers = curl_slist_append(headers, header_buffer);
    }
    if (domain != NULL) {
        snprintf(header_buffer, sizeof(header_buffer), "CDC-Domain: %s", domain);
        headers = curl_slist_append(headers, header_buffer);
    }
    if (area != NULL) {
        snprintf(header_buffer, sizeof(header_buffer), "CDC-Area: %s", area);
        headers = curl_slist_append(headers, header_buffer);
    }
    // 设置请求头
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // 设置响应回调
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_response_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    // 设置超时
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);

    // 执行请求
    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        // 请求失败 - 对应Kotlin的catch (e: Throwable)
        result->type = NET_RESULT_ERROR;
        result->error_message = strdup(curl_easy_strerror(res));
    } else {
        // 获取HTTP状态码
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        result->status_code = (int)response_code;

        // 请求成功 - 对应Kotlin的NetResult.Success
        result->type = NET_RESULT_SUCCESS;
        result->data = response.memory;
        response.memory = NULL; // 防止被释放
    }

    // 清理资源
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (response.memory) {
        free(response.memory);
    }

    return result;
}

// 便捷函数：不带额外请求头的POST请求
NetResult* simple_post(const char* url, const char* data) {
    return post_request(url, data, NULL);
}