//
// Created by bad_g on 2025/9/27.
//
#include "../headFiles/utils/ShutdownHook.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <stdbool.h>

#include "../headFiles/utils/Logger.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

// 最大清理函数数量
#define MAX_CLEANUP_FUNCTIONS 10

// 清理函数数组
static cleanup_function_t cleanup_functions[MAX_CLEANUP_FUNCTIONS];
static int cleanup_count = 0;
static bool shutdown_hook_initialized = false;

// Windows控制台事件处理函数
#ifdef _WIN32
BOOL WINAPI console_ctrl_handler(DWORD ctrl_type) {
    switch (ctrl_type) {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_CLOSE_EVENT:
        case CTRL_LOGOFF_EVENT:
        case CTRL_SHUTDOWN_EVENT:
            LOG_INFO("程序关闭中...");

            // 执行清理操作
            execute_cleanup();

            // 强制退出程序
            // printf("清理完成，程序即将退出...\n");
            exit(0);

            return TRUE;  // 这行实际上不会执行到，但保留以符合函数签名
        default:
            return FALSE;
    }
}
#endif

/**
 * 信号处理函数
 * 对应Kotlin版本shutdown hook的run()方法
 */
void signal_handler(int sig) {
    // 执行清理操作
    execute_cleanup();

    // 退出程序
    exit(0);
}

/**
 * 初始化shutdown hook
 */
void init_shutdown_hook(void) {
    if (shutdown_hook_initialized) {
        return;
    }

#ifdef _WIN32
    // Windows平台：注册控制台事件处理函数
    if (!SetConsoleCtrlHandler(console_ctrl_handler, TRUE)) {
        fprintf(stderr, "警告: 无法注册Windows控制台事件处理函数\n");
    }
#else
    // Unix/Linux平台：注册信号处理函数
    signal(SIGINT, signal_handler);   // Ctrl+C
    signal(SIGTERM, signal_handler);  // 终止信号
    signal(SIGQUIT, signal_handler);  // 退出信号
#endif

    // 注册atexit清理函数
    atexit(execute_cleanup);

    shutdown_hook_initialized = true;
}

/**
 * 注册清理函数
 */
int register_cleanup_function(cleanup_function_t cleanup_func) {
    if (!cleanup_func || cleanup_count >= MAX_CLEANUP_FUNCTIONS) {
        return -1;
    }

    cleanup_functions[cleanup_count++] = cleanup_func;
    return 0;
}
/**
 * 执行所有注册的清理函数
 */
void execute_cleanup(void) {
    static bool cleanup_executed = false;

    // 防止重复执行清理
    if (cleanup_executed) {
        return;
    }
    cleanup_executed = true;

    // 然后执行所有注册的清理函数
    for (int i = 0; i < cleanup_count; i++) {
        if (cleanup_functions[i]) {
            cleanup_functions[i]();
        }
    }
}

/**
 * 优雅退出程序
 */
void graceful_exit(int exit_code) {
    LOG_INFO("程序准备退出，退出码: %d", exit_code);

    // 执行清理操作
    execute_cleanup();

    // 退出程序
    exit(exit_code);
}