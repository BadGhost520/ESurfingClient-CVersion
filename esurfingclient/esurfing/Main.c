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

    g_start_run_tm = get_cur_tm_ms();

    init_shutdown_hook();

    g_prog_status = calloc(1, sizeof(ProgStatus));

    if (init_logger() == false) shut(1);

    if (load_cfg() == false) shut(1);

    NetworkStatus status;
    uint8_t retry = 0;
    do
    {
        status = check_network_status();
        switch (status)
        {
        case REQUEST_SUCCESS:
            LOG_INFO("已连接到互联网");
            sleep_ms(5000);
            break;
        case REQUEST_ERROR:
        case REQUEST_INIT_ERROR:
            if (retry > 5)
            {
                LOG_FATAL("超过重试次数, 退出程序");
                shut(1);
            }
            retry++;
            LOG_ERROR("网络错误, 重试: 第 %" PRIu8 " 次, 最多 5 次", retry);
            sleep_ms(5000);
            break;
        default:
            break;
        }
    } while (status == REQUEST_SUCCESS || status == REQUEST_ERROR);

    get_last_location();

    LOG_DEBUG("开始创建认证线程");
    for (uint8_t i = 0; i < g_prog_cnt; i++)
    {
        g_prog_status[i].thread = sim_thread_create(dialer_app, (void*)(intptr_t)i);
        if (!g_prog_status[i].thread) {
            LOG_FATAL("线程 %" PRIu8 " 创建失败", i);
            shut(1);
        }
        g_prog_status[i].runtime_status.is_running = true;

        retry = 0;
        while (g_prog_status[i].runtime_status.is_authed == false)
        {
            if (retry > 5)
            {
                LOG_FATAL("超过重试次数, 退出程序");
                shut(1);
            }
            retry++;
            LOG_DEBUG("等待配置 %" PRIu8 " 认证完成, 下标 %" PRIu8 ", 等待次数: %" PRIu8 ", 最多 5 次", g_prog_status[i].login_cfg.idx, i, retry);
            sleep_ms(2000);
        }
    }

    LOG_INFO("线程守护开启");
    thread_keep_alive = true;
    while (thread_keep_alive)
    {
        for (uint8_t i = 0; i < g_prog_cnt; i++)
        {
            if (get_cur_tm_ms() - g_prog_status[i].auth_cfg.auth_time >= 172200000 && g_prog_status[i].auth_cfg.auth_time != 0)
            {
                // if (g_prog_status[thread_idx].runtime_status.is_settings_changed)
                // {
                //     LOG_INFO("设置已更改, 正在重启认证");
                //     g_prog_status[thread_idx].runtime_status.is_settings_changed = false;
                // }
                LOG_DEBUG("当前时间戳: %" PRIu64, get_cur_tm_ms());
                LOG_WARN("已登录 2870 分钟 (1 天 23 小时 50 分钟), 为避免被远程服务器踢下线, 正在重新进行认证");
                for (uint8_t j = 0; j < g_prog_cnt; j++)
                {
                    g_prog_status[j].runtime_status.is_need_reset = true;
                    g_prog_status[j].runtime_status.is_running = false;
                    g_prog_status[j].auth_cfg.auth_time = 0;
                    while (g_prog_status[j].runtime_status.is_authed)
                    {
                        if (retry > 5)
                        {
                            LOG_FATAL("超过重试次数, 退出程序");
                            shut(1);
                        }
                        retry++;
                        LOG_DEBUG("等待配置 %" PRIu8 " 登出完成, 下标 %" PRIu8 ", 等待次数: %" PRIu8 ", 最多 5 次", g_prog_status[j].login_cfg.idx, j, retry);
                        sleep_ms(2000);
                    }
                }
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
                LOG_INFO("认证线程 %" PRIu8 " 已退出, 由于线程守护已开启, 将会重新启动此线程", i);
                g_prog_status[i].thread = sim_thread_create(dialer_app, (void*)(intptr_t)i);
                if (!g_prog_status[i].thread) {
                    LOG_FATAL("线程 %" PRIu8 " 创建失败", i);
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
