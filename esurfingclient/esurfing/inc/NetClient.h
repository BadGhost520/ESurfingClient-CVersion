#ifndef ESURFINGCLIENT_NETCLIENT_H
#define ESURFINGCLIENT_NETCLIENT_H

#ifdef __mips__
    #define NO_MIPS16 __attribute__((nomips16))
#else
    #define NO_MIPS16
#endif

#include <stddef.h>

typedef enum {
    REQUEST_ERROR = 0,
    REQUEST_INIT_ERROR = 1,
    REQUEST_WARN = 3,
    REQUEST_HAVE_RES = 200,
    REQUEST_SUCCESS = 204,
    REQUEST_REDIRECT = 302
} NetworkStatus;

typedef struct {
    NetworkStatus status;
    char* body_data;
    size_t body_size;
} HTTPResponse;

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
HTTPResponse post(const char* url, const char* data);

/**
 * @brief 带默认头的 GET
 * @param url 地址
 * @return 响应数据
 *
 */
HTTPResponse get(const char* url);

/**
 * @brief 初始化网络状态检查 CURL
 * @return 初始化状态
 */
bool init_check_curl();

/**
 * @brief 清理网络状态检查 CURL
 */
NO_MIPS16
void clean_check_curl();

/**
 * @brief 检测网络状态
 * @return 网络状态
 */
NetworkStatus check_network_status();

/**
 * @brief 获取所有 ip 的 last_location
 */
void get_last_location();

#endif //ESURFINGCLIENT_NETCLIENT_H
