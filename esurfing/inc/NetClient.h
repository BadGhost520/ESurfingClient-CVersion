#ifndef ESURFINGCLIENT_NETCLIENT_H
#define ESURFINGCLIENT_NETCLIENT_H

#include <stddef.h>

typedef enum {
    REQUEST_ERROR = 0,
    REQUEST_SUCCESS = 1,
    REQUEST_AUTHORIZATION = 2,
    REQUEST_REDIRECT = 3,
    REQUEST_WARNING = 4,
    REQUEST_HAVE_RES = 5,
    INIT_ERROR = 6
} NetworkStatus;

typedef enum
{
    CURL_INIT_SUCCESS = 0,
    CURL_INIT_FAILURE = 1,
} CurlStatus;

typedef struct {
    NetworkStatus status;
    char* body_data;
    size_t body_size;
} HTTPResponse;

/**
 * @brief 带默认头的 POST
 * @param url 地址
 * @param data 数据
 * @return 响应数据
 */
HTTPResponse post_with_header(const char* url, const char* data);

/**
 * @brief 带默认头的 GET
 * @param url 地址
 * @return 响应数据
 */
HTTPResponse get_with_header(const char* url);

/**
 * @brief 获取所有 ip 的 last_location
 */
void get_last_location();

/**
 * @brief 检测认证状态
 * @return 网络状态
 */
NetworkStatus check_auth_status();

/**
 * @brief 检查 IP 是否有效
 * @param ip 要检查的 IP
 * @return 有效性
 */
bool check_ip_validity(const char* ip);

#endif //ESURFINGCLIENT_NETCLIENT_H
