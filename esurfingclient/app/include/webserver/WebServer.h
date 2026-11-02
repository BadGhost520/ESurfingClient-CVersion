#ifndef ESURFINGCLIENT_WEBSERVER_H
#define ESURFINGCLIENT_WEBSERVER_H

#include <stdbool.h>

/**
 * @brief 启动 Web 服务器
 *
 * 桌面端只有多进程这一种形态, Web 服务永远跑在独立的 Web 进程里: 认证状态与
 * "重新认证 / 应用新配置"都通过控制通道 (127.0.0.1) 转给认证进程, 本进程不读
 * 认证线程的共享状态。
 * @return 是否启动成功
 */
bool start_web_server_remote(void);

/**
 * @brief 停止 Web 服务器
 */
void stop_web_server(void);

#endif
