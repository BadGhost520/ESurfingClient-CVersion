//
// NetworkStatus.h - 网络状态检查函数声明
// Created by bad_g on 2025/9/14.
//

#ifndef NETWORKSTATUS_H
#define NETWORKSTATUS_H

typedef enum {
    CONNECTIVITY_SUCCESS = 0,                // 网络已连接，无需认证
    CONNECTIVITY_REQUIRE_AUTHORIZATION = 1,   // 需要进行校园网认证
    CONNECTIVITY_REQUEST_ERROR = 2            // 请求失败或网络错误
} ConnectivityStatus;

/**
 * 网络状态检测函数
 * @return 网络状态
 */
ConnectivityStatus checkStatus();

#endif // NETWORKSTATUS_H