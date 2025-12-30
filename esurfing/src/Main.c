#include <process.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>

#include "headFiles/States.h"
#include "headFiles/webserver/WebServer.h"
#include "headFiles/utils/Shutdown.h"
#include "headFiles/utils/Logger.h"
#include "headFiles/utils/PlatformUtils.h"

int main(const int argc, char* argv[])
{
#ifdef _WIN32
    system("chcp 65001 > nul");
#endif
    int opt;
    bool is_debug = false;
    while ((opt = getopt(argc, argv, "d")) != -1)
    {
        switch (opt)
        {
        case 'd':
            is_debug = true;
            break;
        case '?':
            printf("参数错误: %c\n", optopt);
            return 0;
        default:
            printf("未知错误\n");
            return 0;
        }
    }
    loggerInit(is_debug);
    g_running_time = currentTimeMillis();
    initShutdown();
    startWebServer();
    return 1;
}
