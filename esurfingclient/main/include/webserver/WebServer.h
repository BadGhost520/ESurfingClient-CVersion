#ifndef ESURFINGCLIENT_WEBSERVER_H
#define ESURFINGCLIENT_WEBSERVER_H

#include <stdbool.h>

/**
 * @brief 启动 Web 服务器 (单进程模式: 与认证逻辑同进程, 直接读共享状态)
 */
bool start_web_server();

/**
 * @brief 启动 Web 服务器 (Web 进程模式: 认证状态通过控制通道获取)
 */
bool start_web_server_remote();

/**
 * @brief 停止 Web 服务器
 */
void stop_web_server();

#endif //ESURFINGCLIENT_WEBSERVER_H