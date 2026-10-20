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
 * @brief 释放一次响应里的响应体, 并把指针置空
 *
 * 用完一次响应就调一次: 直接 free(body_data) 忘了置空的话, 同一个 resp
 * 被复用时会重复释放
 * @param resp 响应 (可为 NULL)
 */
void curl_resp_free(curl_resp_t* resp);

/**
 * @brief 检测网络状态
 * @return 网络状态
 */
network_status_t check_network_status(bool connect_only);

#endif
