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

    // g_start_run_tm = get_cur_tm_ms(); // 获取开始运行的时间 (暂弃)

    g_prog_status = calloc(1, sizeof(prog_status_t)); // 初始化 g_prog_status 指针, 分配一个空间

    init_shutdown_hook(); // 初始化关闭钩子

    if (init_logger() == false) shut(1); // 初始化日志系统

    if (load_cfg() == false) shut(1); // 加载配置文件

    /**
     * 检测网络状态
     * 非重定向响应都会持续循环
     */
    NetworkStatus status;
    uint8_t retry = 1;
    do
    {
        status = check_network_status();
        switch (status)
        {
        case REQUEST_SUCCESS:
            LOG_INFO("已连接到互联网");
            sleep_ms(10000);
            break;
        case REQUEST_ERROR:
        case REQUEST_INIT_ERROR:
            if (retry > 5)
            {
                LOG_FATAL("超过最多重试次数, 退出程序");
                shut(1);
            }
            LOG_ERROR("网络错误, 重试: 第 %" PRIu8 " 次, 最多 5 次", retry);
            retry++;
            sleep_ms(5000);
            break;
        default:
            break;
        }
    } while (status != REQUEST_REDIRECT);

    /**
     * 根据配置数创建相应数量的线程
     */
    LOG_DEBUG("开始创建认证线程");
    for (uint8_t i = 0; i < g_prog_cnt; i++)
    {
        g_prog_status[i].thread = sim_thread_create(dialer_app, (void*)(intptr_t)i);
        retry = 1;
        while (!g_prog_status[i].thread)
        {
            if (retry > 5)
            {
                LOG_FATAL("超过重试次数, 退出程序");
                shut(1);
            }
            LOG_ERROR("认证线程 %" PRIu8 " 创建失败, 重试中, 重试次数: %" PRIu8 ", 最多 5 次", i, retry);
            g_prog_status[i].thread = sim_thread_create(dialer_app, (void*)(intptr_t)i);
            retry++;
        }
    }

    /**
     * 线程守护
     * 登录时间检测
     */
    LOG_INFO("线程守护开启");
    thread_keep_alive = true;
    uint64_t tick = get_cur_tm_ms();
    while (thread_keep_alive)
    {
        if (get_cur_tm_ms() - tick >= 30000)
        {
            tick = get_cur_tm_ms();
            LOG_DEBUG("线程守护中");
        }
        for (uint8_t i = 0; i < g_prog_cnt; i++)
        {
            /**
             * 认证时间超过 172200000 毫秒 (1 天 23 时 50 分) 自动重启认证
             */
            if (get_cur_tm_ms() - g_prog_status[i].auth_cfg.auth_time >= 120000 && g_prog_status[i].auth_cfg.auth_time != 0)
            {
                // if (g_prog_status[thread_idx].runtime_status.is_settings_changed)
                // {
                //     LOG_INFO("设置已更改, 正在重启认证");
                //     g_prog_status[thread_idx].runtime_status.is_settings_changed = false;
                // }
                LOG_DEBUG("当前时间戳: %" PRIu64, get_cur_tm_ms());
                LOG_WARN("认证时间超过 172200000 毫秒 (1 天 23 时 50 分), 为避免被远程服务器踢下线, 正在重新进行认证");
                for (uint8_t j = 0; j < g_prog_cnt; j++)
                {
                    g_prog_status[j].runtime_status.is_need_reset = true;
                    retry = 1;
                    while (g_prog_status[j].runtime_status.is_authed)
                    {
                        if (retry > 5)
                        {
                            LOG_FATAL("超过重试次数, 强制退出线程");
                            sim_thread_destroy(g_prog_status[j].thread);
                        }
                        LOG_DEBUG("等待配置 %" PRIu8 " 登出, 下标 %" PRIu8 ", 等待次数: %" PRIu8 ", 最多 5 次", g_prog_status[j].login_cfg.idx, j, retry);
                        retry++;
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
            /**
             * 线程守护
             */
            if (g_prog_status[i].runtime_status.is_running == false)
            {
                int result_code = 0;
                sim_thread_join(g_prog_status[i].thread, &result_code);
                LOG_INFO("认证线程 %" PRIu8 " 已结束, 由于线程守护已开启, 将会重新启动此线程", i);
                g_prog_status[i].thread = sim_thread_create(dialer_app, (void*)(intptr_t)i);
                retry = 1;
                while (!g_prog_status[i].thread)
                {
                    if (retry > 5)
                    {
                        LOG_FATAL("超过重试次数, 退出程序");
                        shut(1);
                    }
                    LOG_ERROR("认证线程 %" PRIu8 " 创建失败, 重试中, 重试次数: %" PRIu8 ", 最多 5 次", i, retry);
                    g_prog_status[i].thread = sim_thread_create(dialer_app, (void*)(intptr_t)i);
                    retry++;
                }
            }
        }
        sleep_ms(10);
    }
    LOG_INFO("线程守护已关闭");

    // get_adapters();

    // startWebServer();

    while (thread_keep_alive == false)
    {
        sleep_ms(1000);
    }
}
