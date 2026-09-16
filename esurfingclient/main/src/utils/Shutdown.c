#include "utils/PlatformUtils.h"
#include "utils/TimeControl.h"
#include "utils/Shutdown.h"
#include "utils/Logger.h"

#include "States.h"

#include <signal.h>
#include <stdlib.h>

#ifndef __OPENWRT__
extern void restart_process();
extern void stop_web_server();
#include "control/Control.h"
#endif

#ifdef _WIN32
#include <windows.h>
extern bool get_service_mode();
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

    LOG_INFO("主程序正在关闭");
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
    control_server_stop(); // 先停止接受控制请求

    if (g_is_webserver_running)
    {
        LOG_INFO("关闭 Web 服务器");
        stop_web_server();
    }
#endif

    if (g_thread_keep_alive)
    {
        LOG_INFO("关闭线程守护");
        g_thread_keep_alive = false;
    }
    time_control_stop(); // 等待时间控制定时线程退出

    LOG_INFO("清理资源中");
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
    (void)ctrlType;
    g_stop_requested = 1;
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

void init_shutdown_hook()
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