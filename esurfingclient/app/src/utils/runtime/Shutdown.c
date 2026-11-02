#include "utils/runtime/Shutdown.h"

#include "core/States.h"

#include "utils/time/TimeControl.h"
#include "utils/runtime/Watchdog.h"
#include "utils/log/Logger.h"

#ifdef _WIN32
#include <windows.h>
extern bool get_service_mode(void);
#else
#include <stdlib.h>
#endif

#ifndef __OPENWRT__
extern void restart_process(void);
extern void stop_web_server(void);
#include "control/Control.h"

#ifndef __OPENWRT__
#include "net/NetClient.h"
#include "net/NetTraffic.h"
#include "webserver/HotspotControl.h"
#endif
#endif

void shut(const int8_t exit_code)
{
    // 关闭流程只允许进入一次
    // (Web 端触发的重启与信号/错误触发的关闭可能同时发生)
    static volatile bool shutting_down = false;
    if (shutting_down)
    {
        LOG_WARN("程序已在关闭流程中, 忽略重复的关闭请求");
        return;
    }
    shutting_down = true;

    /**
     * 关闭流程一开始就停掉看门狗: 接下来主循环不再打卡,
     * 不停的话它会把正常退出误判成卡死, 还会盖掉真正的退出原因
     */
    watchdog_stop();

    LOG_DEBUG("主程序正在关闭");
    g_need_exit = true;

    /**
     * 监管进程的收尾是"先停子进程再退出", 而且它自己装了信号处理,
     * 所以这里只置退出标志, 具体的关闭顺序交给 work_supervisor() 处理。
     * (Windows 服务收到 SCM 的停止请求时也是走这里)
     */
    if (g_prog_role == ROLE_SUPERVISOR)
    {
        return;
    }

#ifndef __OPENWRT__
    /**
     * 先停掉网速采样与热点状态线程: 它们会写日志, 必须赶在日志收尾之前停干净
     * (热点那边可能还有一次 PowerShell 在跑, 它是独立进程, 自己会结束)
     */
#ifndef __OPENWRT__
    hotspot_control_stop();
    net_traffic_stop();
    net_probe_stop();
#endif

    control_server_stop(); // 先停止接受控制请求

    if (g_is_webserver_running)
    {
        LOG_DEBUG("关闭 Web 服务器");
        stop_web_server();
    }
#endif

    if (g_thread_keep_alive)
    {
        LOG_DEBUG("关闭线程守护");
        g_thread_keep_alive = false;
    }
    time_control_stop(); // 等待时间控制定时线程退出

    LOG_DEBUG("清理资源中");
    LOG_DEBUG("关闭线程");
    for (uint8_t i = 0; i < g_prog_cnt; i++)
    {
        int result_code = 0;
        g_prog_status[i].runtime_status.is_running = false;
        if (g_prog_status[i].thread != NULL)
        {
            sim_thread_join(g_prog_status[i].thread, &result_code);
            LOG_DEBUG("认证线程 %" PRIu8 " 退出, 退出码: %d", i, result_code);
        }
    }

    LOG_INFO("退出程序, 退出码: %" PRIu8, exit_code);
    clean_logger();
    g_shutdown_done = 1; // 通知控制台处理线程: 收尾已经做完了

#ifdef _WIN32
    if (get_service_mode())
    {
        longjmp(g_exit_jmp, 1);
    }
#endif

#ifndef __OPENWRT__
    if (g_need_restart)
    {
        restart_process();
    }
#endif

    exit(exit_code);
}

#ifdef _WIN32
/**
 * Windows 控制台事件处理
 *
 * 只置标志: 这个处理函数跑在系统另起的线程上, 而且随时可能被强杀,
 * 在这里做 join / 打日志 / rename / exit 都不安全。
 * 真正的关闭动作由各角色的主循环看到标志后执行
 */
static BOOL WINAPI console_handler(const DWORD ctrlType)
{
    g_stop_requested = 1;

    /**
     * 关窗口 / 注销 / 关机: 系统只给几秒, 而且处理函数一返回进程就会被结束。
     *
     * 这里先等主线程把收尾跑完 (登出、停线程、日志归档), 最多约 4.5 秒;
     * 然后必须 return FALSE 交回默认处理 —— 返回 TRUE 等于告诉系统"我自己处理了",
     * 系统就不结束进程, 进程会带着"没有控制台"的状态留在后台 (关终端关不掉就是这个原因)
     */
    if (ctrlType == CTRL_CLOSE_EVENT || ctrlType == CTRL_LOGOFF_EVENT || ctrlType == CTRL_SHUTDOWN_EVENT)
    {
        for (int i = 0; i < 450 && g_shutdown_done == 0; i++)
        {
            Sleep(10);
        }

        // 收尾跑完之后主线程还要 exit(): 再让出几百毫秒, 别抢在它前面被系统杀掉
        if (g_shutdown_done != 0) Sleep(300);

        return FALSE;
    }

    // Ctrl+C / Ctrl+Break: 只置标志, 由主循环走优雅关闭
    return TRUE;
}

#else
/**
 * Linux/Unix 信号处理
 *
 * 只置标志: 信号处理函数里不能做 join 线程、打日志、rename、exit
 * 这些不是 async-signal-safe 的事 (原来这里直接调 shut(), 有死锁风险)。
 * 真正的关闭动作由各角色的主循环看到标志后执行
 */
static void signal_handler(const int sig)
{
    (void)sig;
    g_stop_requested = 1;
}

#endif

void init_shutdown_hook(void)
{
#ifdef _WIN32
    if (SetConsoleCtrlHandler(console_handler, TRUE) == 0)
    {
        fprintf(stderr, "[ERROR] 设置控制台事件处理失败\n");
        exit(1);
    }
#else
    if (signal(SIGINT, signal_handler) == SIG_ERR)
    {
        fprintf(stderr, "[ERROR] 信号 SIGINT 设置失败\n");
        exit(1);
    }
    if (signal(SIGTERM, signal_handler) == SIG_ERR)
    {
        fprintf(stderr, "[ERROR] 信号 SIGTERM 设置失败\n");
        exit(1);
    }
    if (signal(SIGHUP, signal_handler) == SIG_ERR)
    {
        fprintf(stderr, "[ERROR] 信号 SIGHUP 设置失败\n");
        exit(1);
    }
    if (signal(SIGQUIT, signal_handler) == SIG_ERR)
    {
        fprintf(stderr, "[ERROR] 信号 SIGQUIT 设置失败\n");
        exit(1);
    }
#endif
}