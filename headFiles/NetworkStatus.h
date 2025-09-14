//
// NetworkStatus.h - 网络状态检查函数声明
// Created by bad_g on 2025/9/14.
//

#ifndef NETWORKSTATUS_H
#define NETWORKSTATUS_H

// 网络状态检查函数
// 访问 http://connect.rom.miui.com/generate_204
// 返回值: 0-返回204, 1-返回304, 2-其他状态码或请求失败
int checkNetworkStatus(void);

#endif // NETWORKSTATUS_H