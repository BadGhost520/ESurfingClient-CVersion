//
// NetworkStatus.h - 网络状态检查函数声明
// Created by bad_g on 2025/9/14.
//

#ifndef NETWORKSTATUS_H
#define NETWORKSTATUS_H

typedef enum {
    RequestSuccess = 0,
    RequestAuthorization = 1,
    RequestError = 2,
    InitError = 3
} ConnectivityStatus;

/**
 * 检测网络状态
 * @return 网络状态
 */
ConnectivityStatus checkStatus();

#endif // NETWORKSTATUS_H