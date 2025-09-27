//
// Created by bad_g on 2025/9/27.
//
#include "../headFiles/utils/ShutdownHook.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#ifdef _WIN32
#include <windows.h>
#endif

// 用户自定义清理函数指针
void (*user_cleanup_function)(void) = NULL;

// atexit清理函数
void cleanup_on_exit(void) {
    if (user_cleanup_function != NULL) {
        user_cleanup_function();
    }
}

// 信号处理函数
void signal_handler(int sig) {
    printf("程序正在退出...\n");

    // 如果用户设置了清理函数，则调用
    if (user_cleanup_function != NULL) {
        user_cleanup_function();
    }

    exit(0);
}

#ifdef _WIN32
// Windows控制台事件处理
BOOL WINAPI console_ctrl_handler(DWORD ctrl_type) {
    switch (ctrl_type) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        printf("程序正在退出...\n");
        if (user_cleanup_function != NULL) {
            user_cleanup_function();
        }
        return TRUE;
    }
    return FALSE;
}
#endif

// 初始化shutdown hook - 对应Kotlin的addShutdownHook
void addShutdownHook(void) {
#ifdef _WIN32
    // Windows平台
    SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
#else
    // Unix/Linux平台
    signal(SIGINT, signal_handler);   // Ctrl+C
    signal(SIGTERM, signal_handler);  // 终止信号
    signal(SIGQUIT, signal_handler);  // 退出信号
#endif

    // 注册atexit清理函数
    atexit(cleanup_on_exit);
}