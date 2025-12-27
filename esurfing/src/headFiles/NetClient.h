#ifndef ESURFINGCLIENT_NETCLIENT_H
#define ESURFINGCLIENT_NETCLIENT_H

typedef enum {
    REQUEST_ERROR = 0,
    REQUEST_SUCCESS = 1,
    REQUEST_AUTHORIZATION = 2,
    INIT_ERROR = 3
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
 * 简单 POST 函数
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

/**
 * 简单 GET 函数
 * @param url 网址
 * @return 网络状态
 */
NetworkStatus simGet(char* url);

#endif //ESURFINGCLIENT_NETCLIENT_H