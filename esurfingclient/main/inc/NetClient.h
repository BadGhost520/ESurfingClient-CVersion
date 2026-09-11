#ifndef ESURFINGCLIENT_NETCLIENT_H
#define ESURFINGCLIENT_NETCLIENT_H

#include <curl/curl.h>

#define HTTP_OK 200
#define HTTP_NO_CONTENT 204
#define HTTP_MOVED_PERMANENTLY 301
#define HTTP_FOUND 302

typedef enum {
    STATUS_OK = 0,
    STATUS_NEED_AUTH = 1,
    STATUS_ERROR = 2,
    STATUS_INIT_ERROR = 3,
} network_status_t;

typedef enum
{
    CONNECT_INTERNET = 0,
    CONNECT_AUTH_SERVER = 1,
    CONNECT_ERROR = 2,
} connection_status_t;

typedef struct {
    network_status_t status;
    long http_code;
    CURLcode curl_code;
    char* body_data;
    size_t body_size;
} curl_resp_t;

/**
 * @brief 截取 URL 中指定参数
 * @param url URL 地址
 * @param search_str_start 要查找的参数名
 * @return 查找到的参数
 */
char* extract_url_param(const char* url, const char* search_str_start);

/**
 * @brief 带默认头的 POST
 * @param url 地址
 * @param data 数据
 * @return 响应数据
 */
curl_resp_t post(const char* url, const char* data);

/**
 * @brief 带默认头的 GET
 * @param url 地址
 * @param connect_only 是否仅连接服务器
 * @return 响应数据
 *
 */
curl_resp_t get(const char* url, bool connect_only);

/**
 * @brief 检测网络状态
 * @return 网络状态
 */
network_status_t check_network_status(bool connect_only);

/**
 * @brief 获取所有 ip 的 last_location
 * @return 网络状态
 */
bool get_last_location();

#endif //ESURFINGCLIENT_NETCLIENT_H
