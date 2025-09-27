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

// 网络状态检查函数
// 访问 http://connect.rom.miui.com/generate_204
// 返回值: 0-返回204, 1-返回304, 2-其他状态码或请求失败
ConnectivityStatus detectConfig(void);

#endif // NETWORKSTATUS_H