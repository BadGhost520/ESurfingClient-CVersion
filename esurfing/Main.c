#include "webserver/WebServer.h"
#include "utils/PlatformUtils.h"
#include "utils/Shutdown.h"
#include "NetClient.h"
#include "States.h"

#include <stdlib.h>

int main()
{
    g_running_time = currentTimeMillis();

#ifdef _WIN32
    system("chcp 65001 >nul");
#endif

    loadJSON();

    initShutdown();

    checkNetworkStatus();

    getAdapters();

    startWebServer();
}
