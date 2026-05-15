#ifndef ESURFINGCLIENT_SERVICE_H
#define ESURFINGCLIENT_SERVICE_H

#include <stdbool.h>

#ifdef _WIN32
#include <windows.h>

#define SERVICE_NAME "ESurfingClient"
#define DISPLAY_NAME "ESurfingClient Auth Service"
#endif

#ifdef _WIN32

bool get_service_mode();

void WINAPI ServiceMain(DWORD argc, LPTSTR *argv);

#endif

/**
 * 安装系统服务并设置为开机自启
 * 需要管理员/root 权限
 * @return 0 成功，-1 失败
 */
int service_install();

/**
 * 卸载系统服务
 * 需要管理员/root 权限
 * @return 0 成功，-1 失败
 */
int service_uninstall();

#endif //ESURFINGCLIENT_SERVICE_H
