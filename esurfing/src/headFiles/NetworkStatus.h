//
// NetworkStatus.h - 网络状态检查函数声明
// Created by bad_g on 2025/9/14.
//

#ifndef NETWORKSTATUS_H
#define NETWORKSTATUS_H

typedef enum {
    CONNECTIVITY_SUCCESS = 0,
    CONNECTIVITY_REQUIRE_AUTHORIZATION = 1,
    CONNECTIVITY_REQUEST_ERROR = 2
} ConnectivityStatus;

/*
 * 检测网络状态
 */
ConnectivityStatus checkStatus();

#endif // NETWORKSTATUS_H