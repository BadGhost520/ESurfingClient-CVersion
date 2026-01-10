#include <stdio.h>

#include "headFiles/webserver/WebServer.h"
#include "headFiles/utils/PlatformUtils.h"
#include "headFiles/utils/Shutdown.h"
#include "headFiles/utils/Logger.h"
#include "headFiles/DialerClient.h"
#include "headFiles/NetClient.h"
#include "headFiles/States.h"

int main()
{
#ifdef _WIN32
    system("chcp 65001 >nul");
#endif
    loadIni();
    g_running_time = currentTimeMillis();
    initShutdown();
    checkNetworkStatus();
    getAdapters();
    while (school_connection_status[0].ip[0] == '\0' && school_connection_status[1].ip[0] == '\0')
    {
        LOG_ERROR("请接入校园网");
        shut(1);
    }
    for (int i = 0; i < MAX_DIALER_COUNT; i++)
    {
        args[i].thread_index = i;
        if (school_connection_status[i].ip[0] != '\0')
        {
            LOG_DEBUG("线程 %d 获取到 IP: %s", i + 1, school_connection_status[i].ip);
            snprintf(args[i].ip, IP_LENGTH, "%s", school_connection_status[i].ip);
            args[i].can_run = true;
        }
        else
        {
            LOG_WARN("线程 %d 未获取到 IP, 将会禁用认证", i + 1);
            args[i].can_run = false;
        }
        createThread(dialerApp, (void*)(intptr_t)i);
    }
    threadAutoStart();
    startWebServer();
}
