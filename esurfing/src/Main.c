#include <process.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>

#include "headFiles/webserver/WebServer.h"
#include "headFiles/utils/Shutdown.h"
#include "headFiles/utils/Logger.h"

int main(const int argc, char* argv[])
{
#ifdef _WIN32
    system("chcp 65001 > nul");
#endif
    int opt;
    while ((opt = getopt(argc, argv, "ds")) != -1)
    {
        switch (opt)
        {
        case 'd':
            g_logger_settings.is_debug = true;
            break;
        case 's':
            g_logger_settings.is_small_device = true;
            break;
        case '?':
            printf("参数错误: %c\n", optopt);
            return 1;
        default:
            printf("未知错误\n");
        }
    }
    loggerInit();
    initShutdown();
    startWebServer();
}
