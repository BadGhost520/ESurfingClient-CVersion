#ifndef ESURFINGCLIENT_WEBSERVER_H
#define ESURFINGCLIENT_WEBSERVER_H

// TODO 记得修改版本号
#define VERSION "v2.0.0-r1"

extern uint8_t is_webserver_running;

/**
 * 启动Web服务器
 */
void startWebServer();

/**
 * 停止Web服务器
 */
void stopWebServer();

#endif //ESURFINGCLIENT_WEBSERVER_H