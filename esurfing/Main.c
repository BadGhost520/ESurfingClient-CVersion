// #include "webserver/WebServer.h"
#include "utils/PlatformUtils.h"
#include "utils/Shutdown.h"
#include "utils/Logger.h"
#include "NetClient.h"
#include "States.h"

#include <stdlib.h>

#include "DialerClient.h"

int main()
{
#ifdef _WIN32
    system("chcp 65001 >nul");
#endif

    g_running_tm = get_cur_tm_ms();

    g_prog_status = calloc(1, sizeof(ProgStatus));

    init_logger();

    if (load_cfg() == false) shut(0);

    init_shutdown_hook();

    get_last_location();

    for (g_prog_idx = 0; g_prog_idx < g_prog_cnt; g_prog_idx++)
    {
        g_prog_status[g_prog_idx].runtime_status.is_running = true;
    }

    for (g_prog_idx = 0; g_prog_idx < g_prog_cnt && g_prog_status[g_prog_idx].runtime_status.is_running; g_prog_idx++)
    {
        while (g_shut_lock) sleep_ms(1000);
        if (g_prog_idx == g_prog_cnt) g_prog_idx = 0;
        dialer_app();
    }

    // get_adapters();

    // startWebServer();

    shut(0);
}
