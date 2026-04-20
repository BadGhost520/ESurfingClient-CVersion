// #include "webserver/WebServer.h"
#include "utils/PlatformUtils.h"
#include "utils/Shutdown.h"
#include "utils/Logger.h"
#include "NetClient.h"
#include "States.h"

#include <stdlib.h>

int main()
{
#ifdef _WIN32
    system("chcp 65001 >nul");
#endif

    g_running_tm = cur_tm_ms();

    g_prog_status = calloc(1, sizeof(ProgStatus));

    init_logger(LOG_LEVEL_DEBUG);

    if (load_cfg() == false) shut(0);

    init_shutdown_hook();

    get_last_location();

    // get_adapters();

    // startWebServer();

    shut(0);
}
