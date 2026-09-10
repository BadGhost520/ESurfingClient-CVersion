#ifndef ESURFINGCLIENT_NETCLIENT_H
#define ESURFINGCLIENT_NETCLIENT_H

#include <curl/curl.h>

#define HTTP_OK 200
#define HTTP_NO_CONTENT 204
#define HTTP_MOVED_PERMANENTLY 301
#define HTTP_FOUND 302

typedef enum {
    STATUS_CONNECT_INTERNET = 0,
    STATUS_CONNECT_AUTH_SERVER = 1,
    STATUS_WARN = 2,
    STATUS_ERROR = 3,
    STATUS_INIT_ERROR = 4,
} network_status_t;

typedef struct {
    network_status_t status;
    long http_code;
    CURLcode curl_code;
    char* body_data;
    size_t body_size;
} resp_t;

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
resp_t post(const char* url, const char* data);

/**
 * @brief 带默认头的 GET
 * @param url 地址
 * @param connect_only 是否仅连接服务器
 * @return 响应数据
 *
 */
resp_t get(const char* url, bool connect_only);

/**
 * @brief 检测网络状态
 * @return 网络状态
 */
resp_t check_network_status();

/**
 * @brief 获取所有 ip 的 last_location
 * @return 网络状态
 */
network_status_t get_last_location();

#endif //ESURFINGCLIENT_NETCLIENT_H
