#include "headFiles/webserver/WebServer.h"
#include "headFiles/utils/PlatformUtils.h"
#include "headFiles/utils/Shutdown.h"
#include "headFiles/States.h"

int main()
{
#ifdef _WIN32
    system("chcp 65001 >nul");
#endif
    loadIni();
    g_running_time = currentTimeMillis();
    initShutdown();
    startWebServer();
}
