// #include "webserver/WebServer.h"
#include "utils/PlatformUtils.h"
#include "utils/Shutdown.h"
#include "utils/Logger.h"
#include "DialerClient.h"
#include "NetClient.h"
#include "States.h"

#include <stdlib.h>

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

    while (check_network_status() == REQUEST_SUCCESS)
    {
        LOG_INFO("已连接到互联网");
        sleep_ms(5000);
    }

    get_last_location();

    LOG_DEBUG("开始创建认证线程");
    for (uint8_t i = 0; i < g_prog_cnt; i++)
    {
        g_prog_status[i].thread = sim_thread_create(dialer_app, (void*)(intptr_t)i);
        if (!g_prog_status[i].thread) {
            LOG_ERROR("线程 %" PRIu8 " 创建失败", i);
            shut(1);
        }
        g_prog_status[i].runtime_status.is_running = true;
        sleep_ms(1000);
    }

    LOG_DEBUG("线程保活开启");
    thread_keep_alive = true;
    while (thread_keep_alive)
    {
        for (uint8_t i = 0; i < g_prog_cnt; i++)
        {
            if (get_cur_tm_ms() - g_prog_status[i].auth_cfg.auth_time >= 60000 && g_prog_status[i].auth_cfg.auth_time != 0)
            {
                //  172200000
                // if (g_prog_status[thread_idx].runtime_status.is_settings_changed)
                // {
                //     LOG_INFO("设置已更改, 正在重启认证");
                //     g_prog_status[thread_idx].runtime_status.is_settings_changed = false;
                // }
                LOG_DEBUG("当前时间戳 (毫秒): %" PRIu64, get_cur_tm_ms());
                LOG_WARN("已登录 2870 分钟 (1 天 23 小时 50 分钟), 为避免被远程服务器踢下线, 正在重新进行认证");
                for (uint8_t j = 0; j < g_prog_cnt; j++)
                {
                    g_prog_status[j].runtime_status.is_need_reset = true;
                    g_prog_status[j].auth_cfg.auth_time = 0;
                }
                sleep_ms(5000);
                break;
            }
            // else if (g_prog_status[thread_idx].runtime_status.is_settings_changed)
            // {
            //     LOG_INFO("设置已更改, 正在重启认证");
            //     reset();
            //     g_prog_status[thread_idx].runtime_status.is_settings_changed = false;
            // }
        }
        for (uint8_t i = 0; i < g_prog_cnt; i++)
        {
            if (!g_prog_status[i].runtime_status.is_running)
            {
                int result_code = 0;
                sim_thread_join(g_prog_status[i].thread, &result_code);
                LOG_DEBUG("线程 %" PRIu8 " 已退出, 线程保活激活中, 将会重新启动线程", i);
                g_prog_status[i].thread = sim_thread_create(dialer_app, (void*)(intptr_t)i);
                if (!g_prog_status[i].thread) {
                    LOG_ERROR("线程 %" PRIu8 " 创建失败", i);
                    shut(1);
                }
                g_prog_status[i].runtime_status.is_running = true;
            }
        }
        sleep_ms(1);
    }

    // get_adapters();

    // startWebServer();

    while (!thread_keep_alive)
    {
        sleep_ms(1000);
    }
}
