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
    LoggerSettings logger_settings = {
        .is_debug = false,
        .is_small_device = true
    };
    while ((opt = getopt(argc, argv, "ds")) != -1)
    {
        switch (opt)
        {
        case 'd':
            logger_settings.is_debug = true;
            break;
        case 's':
            logger_settings.is_small_device = true;
            break;
        case '?':
            printf("参数错误: %c\n", optopt);
            return 0;
        default:
            printf("未知错误\n");
            return 0;
        }
    }
    loggerInit(logger_settings);
    initShutdown();
    startWebServer();
    return 1;
}
