#ifndef ESURFINGCLIENT_NETCLIENT_H
#define ESURFINGCLIENT_NETCLIENT_H

#include "Definition.h"

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

#endif //ESURFINGCLIENT_NETCLIENT_H
