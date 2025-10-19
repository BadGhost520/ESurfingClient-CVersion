//
// Created by bad_g on 2025/9/23.
//

#ifndef ESURFINGCLIENT_NETCLIENT_H
#define ESURFINGCLIENT_NETCLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

    // 最大长度定义
#define MAX_URL_LENGTH 512
#define MAX_HEADER_LENGTH 256
#define MAX_DATA_LENGTH 4096
#define MAX_RESPONSE_LENGTH 8192
#define MAX_HEADERS_COUNT 20

typedef struct {
    char* memory;
    size_t size;
} HTTPResponse;

typedef enum {
    NET_RESULT_SUCCESS,
    NET_RESULT_ERROR
} NetResultType;

typedef struct {
    NetResultType type;
    char* data;
    size_t dataSize;
    char* errorMessage;
    int statusCode;
} NetResult;

typedef struct {
    char key[MAX_HEADER_LENGTH];
    char value[MAX_HEADER_LENGTH];
} HeaderPair;

typedef struct {
    HeaderPair headers[MAX_HEADERS_COUNT];
    int count;
} ExtraHeaders;

/**
 * 释放网络返回值函数
 * @param result 网络返回值
 */
void freeNetResult(NetResult* result);

size_t writeResponseCallback(const void *contents, size_t size, size_t nmemb, HTTPResponse *response);

/**
 * POST 函数
 * @param url 网址
 * @param data 数据
 * @return 网络返回值
 */
NetResult* simPost(const char* url, const char* data);

#ifdef __cplusplus
}
#endif

#endif //ESURFINGCLIENT_NETCLIENT_H