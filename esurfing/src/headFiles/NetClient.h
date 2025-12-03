//
// Created by bad_g on 2025/9/23.
//

#ifndef ESURFINGCLIENT_NETCLIENT_H
#define ESURFINGCLIENT_NETCLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    RequestSuccess = 1,
    RequestAuthorization = 2,
    RequestError = 3,
    InitError = 4
} NetworkStatus;

typedef struct {
    NetworkStatus status;
    char* data;
    size_t dataSize;
} HTTPResponse;

/**
 * 释放网络返回值函数
 * @param result 网络返回值
 */
void freeResult(HTTPResponse* result);

/**
 * POST 函数
 * @param url 网址
 * @param data 数据
 * @return 网络返回值
 */
HTTPResponse* simPost(const char* url, const char* data);

/**
 * 检测网络状态
 * @return 网络状态
 */
NetworkStatus checkNetworkStatus();

#ifdef __cplusplus
}
#endif

#endif //ESURFINGCLIENT_NETCLIENT_H