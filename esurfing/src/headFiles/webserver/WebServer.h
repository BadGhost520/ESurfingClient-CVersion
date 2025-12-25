#ifndef ESURFINGCLIENT_WEBSERVER_H
#define ESURFINGCLIENT_WEBSERVER_H

extern int is_webserver_running;
extern int is_settings_changed;

/**
 * 启动Web服务器
 */
void startWebServer();

/**
 * 停止Web服务器
 */
void stopWebServer();

#endif //ESURFINGCLIENT_WEBSERVER_H