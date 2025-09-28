//
// Created by bad_g on 2025/9/27.
//

#ifndef ESURFINGCLIENT_SHUTDOWNHOOK_H
#define ESURFINGCLIENT_SHUTDOWNHOOK_H
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

    /**
     * C语言版本的shutdown hook实现
     * 对应Kotlin/Java的Runtime.getRuntime().addShutdownHook()
     *
     * 功能：
     * 1. 捕获程序退出信号（SIGINT, SIGTERM等）
     * 2. 在程序退出前执行清理操作
     * 3. 确保资源正确释放
     */

    // 清理函数类型定义
    typedef void (*cleanup_function_t)(void);

    /**
     * 初始化shutdown hook
     * 注册信号处理函数，设置程序退出时的清理机制
     */
    void init_shutdown_hook(void);

    /**
     * 注册清理函数
     * @param cleanup_func 要在程序退出时执行的清理函数
     * @return 0成功，-1失败
     */
    int register_cleanup_function(cleanup_function_t cleanup_func);

    /**
     * 信号处理函数
     * 处理SIGINT (Ctrl+C), SIGTERM等退出信号
     * @param sig 信号编号
     */
    void signal_handler(int sig);

    /**
     * 执行所有注册的清理函数
     * 对应Kotlin版本shutdown hook中的清理逻辑
     */
    void execute_cleanup(void);

    /**
     * 优雅退出程序
     * @param exit_code 退出码
     */
    void graceful_exit(int exit_code);

#ifdef __cplusplus
}
#endif

#endif //ESURFINGCLIENT_SHUTDOWNHOOK_H