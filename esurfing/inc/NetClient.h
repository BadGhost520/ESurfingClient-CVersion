#ifndef ESURFINGCLIENT_NETCLIENT_H
#define ESURFINGCLIENT_NETCLIENT_H

#include <stddef.h>

#define SCHOOL_NETWORK_SYMBOL 8
#define LOCATION_LENGTH 256
#define SCHOOL_ID_LENGTH 8
#define DOMAIN_LENGTH 16
#define AREA_LENGTH 8

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

extern __thread char latest_location[LOCATION_LENGTH];
extern __thread char check_location[LOCATION_LENGTH];

extern char school_network_symbol[SCHOOL_NETWORK_SYMBOL];

/**
 * 会话 POST
 * @param url 网址
 * @param data 数据
 * @return 响应数据
 */
HTTPResponse sessionPost(const char* url, const char* data);

/**
 * 检查网络状态
 * @return 网络状态
 */
NetworkStatus checkNetworkStatus();

/**
 * 检测认证状态
 * @return 网络状态
 */
NetworkStatus checkAuthStatus();

#endif //ESURFINGCLIENT_NETCLIENT_H