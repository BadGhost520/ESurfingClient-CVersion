//
// Created by bad_g on 2025/9/28.
//

#ifndef ESURFINGCLIENT_LOGGER_H
#define ESURFINGCLIENT_LOGGER_H

/**
 * C语言日志系统 - 头文件
 *
 * 功能特性：
 * - 支持多种日志级别 (DEBUG, INFO, WARN, ERROR, FATAL)
 * - 支持控制台和文件输出
 * - 自动添加时间戳
 * - 线程安全（可选）
 * - 日志文件轮转
 * - 颜色输出支持
 *
 * 作者: ESurfingDialer项目
 * 版本: 1.0
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <limits.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

// ========== 日志级别定义 ==========
typedef enum {
    LOG_LEVEL_DEBUG = 0,    // 调试信息
    LOG_LEVEL_INFO  = 1,    // 一般信息
    LOG_LEVEL_WARN  = 2,    // 警告信息
    LOG_LEVEL_ERROR = 3,    // 错误信息
    LOG_LEVEL_FATAL = 4,    // 致命错误
    LOG_LEVEL_OFF   = 5     // 关闭日志
} LogLevel;

// ========== 日志输出目标 ==========
typedef enum {
    LOG_TARGET_CONSOLE = 1,     // 控制台输出
    LOG_TARGET_FILE    = 2,     // 文件输出
    LOG_TARGET_BOTH    = 3      // 同时输出到控制台和文件
} LogTarget;

// ========== 日志配置结构体 ==========
typedef struct {
    LogLevel    level;              // 当前日志级别
    LogTarget   target;             // 输出目标
    char        log_file[PATH_MAX]; // 日志文件路径
    FILE*       file_handle;        // 文件句柄
    int        enable_color;       // 是否启用颜色输出
    int        enable_timestamp;   // 是否启用时间戳
    int        enable_thread_safe; // 是否启用线程安全
    size_t      max_file_size;      // 最大文件大小（字节）
    int         max_backup_files;   // 最大备份文件数
} LoggerConfig;

// ========== 颜色代码定义 ==========
#ifdef _WIN32
    #define COLOR_RESET     ""
    #define COLOR_DEBUG     ""
    #define COLOR_INFO      ""
    #define COLOR_WARN      ""
    #define COLOR_ERROR     ""
    #define COLOR_FATAL     ""
#else
    #define COLOR_RESET     "\033[0m"
    #define COLOR_DEBUG     "\033[36m"      // 青色
    #define COLOR_INFO      "\033[32m"      // 绿色
    #define COLOR_WARN      "\033[33m"      // 黄色
    #define COLOR_ERROR     "\033[31m"      // 红色
    #define COLOR_FATAL     "\033[35m"      // 紫色
#endif

// ========== 全局日志配置 ==========
extern LoggerConfig g_logger_config;

// ========== 核心函数声明 ==========

/**
 * 初始化日志系统
 * @param level 日志级别
 * @param target 输出目标
 * @return 0成功，-1失败
 */
int logger_init(LogLevel level, LogTarget target);

/**
 * 清理日志系统资源
 */
void logger_cleanup(void);

/**
 * 设置日志级别
 * @param level 新的日志级别
 */
void logger_set_level(LogLevel level);

/**
 * 设置是否启用颜色输出
 * @param enable true启用，false禁用
 */
void logger_set_color(int enable);

/**
 * 设置是否启用时间戳
 * @param enable true启用，false禁用
 */
void logger_set_timestamp(int enable);

/**
 * 设置文件大小限制和备份数量
 * @param max_size 最大文件大小（字节）
 * @param max_backups 最大备份文件数
 */
void logger_set_rotation(size_t max_size, int max_backups);

/**
 * 核心日志输出函数
 * @param level 日志级别
 * @param file 源文件名
 * @param line 行号
 * @param func 函数名
 * @param format 格式化字符串
 * @param ... 可变参数
 */
void logger_log(LogLevel level, const char* file, int line, const char* func, const char* format, ...);

/**
 * 获取日志级别字符串
 * @param level 日志级别
 * @return 级别字符串
 */
const char* logger_level_string(LogLevel level);

/**
 * 获取日志级别颜色
 * @param level 日志级别
 * @return 颜色代码字符串
 */
const char* logger_level_color(LogLevel level);

/**
 * 检查文件大小并执行轮转
 */
void logger_rotate_if_needed(void);

// ========== 便捷宏定义 ==========

#define LOG_DEBUG(format, ...) \
    logger_log(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

#define LOG_INFO(format, ...) \
    logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

#define LOG_WARN(format, ...) \
    logger_log(LOG_LEVEL_WARN, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

#define LOG_ERROR(format, ...) \
    logger_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

#define LOG_FATAL(format, ...) \
    logger_log(LOG_LEVEL_FATAL, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

// ========== 条件日志宏 ==========

#define LOG_DEBUG_IF(condition, format, ...) \
    do { if (condition) LOG_DEBUG(format, ##__VA_ARGS__); } while(0)

#define LOG_INFO_IF(condition, format, ...) \
    do { if (condition) LOG_INFO(format, ##__VA_ARGS__); } while(0)

#define LOG_WARN_IF(condition, format, ...) \
    do { if (condition) LOG_WARN(format, ##__VA_ARGS__); } while(0)

#define LOG_ERROR_IF(condition, format, ...) \
    do { if (condition) LOG_ERROR(format, ##__VA_ARGS__); } while(0)

#define LOG_FATAL_IF(condition, format, ...) \
    do { if (condition) LOG_FATAL(format, ##__VA_ARGS__); } while(0)

// ========== 特殊用途宏 ==========

// 函数进入/退出跟踪
#define LOG_FUNCTION_ENTER() LOG_DEBUG("进入函数: %s", __func__)
#define LOG_FUNCTION_EXIT()  LOG_DEBUG("退出函数: %s", __func__)

// 变量值打印
#define LOG_VAR_INT(var)    LOG_DEBUG(#var " = %d", var)
#define LOG_VAR_STR(var)    LOG_DEBUG(#var " = %s", var)
#define LOG_VAR_PTR(var)    LOG_DEBUG(#var " = %p", var)

#endif // LOGGER_H

#endif //ESURFINGCLIENT_LOGGER_H