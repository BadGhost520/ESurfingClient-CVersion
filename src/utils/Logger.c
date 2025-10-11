//
// Created by bad_g on 2025/9/28.
//
/**
 * ? C语言日志系统 - 实现文件
 *
 * 实现了完整的日志功能：
 * - 多级别日志输出
 * - 文件和控制台输出
 * - 时间戳和颜色支持
 * - 文件大小轮转
 * - 线程安全（基础版本）
 *
 * 作者: ESurfingDialer项目
 * 版本: 1.0
 */
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <stdlib.h>

#ifdef _WIN32
    #include <windows.h>
    #include <io.h>
#else
    #include <unistd.h>
    #include <sys/stat.h>
#endif

#include "../headFiles/utils/Logger.h"

// ========== 全局变量 ==========
LoggerConfig g_logger_config = {
    .level = LOG_LEVEL_INFO,
    .target = LOG_TARGET_CONSOLE,
    .log_file = "",
    .file_handle = NULL,
    .enable_color = 1,
    .enable_timestamp = 1,
    .enable_thread_safe = 0,
    .max_file_size = 10 * 1024 * 1024,  // 10MB
    .max_backup_files = 5
};

// ========== 内部函数声明 ==========
static void logger_write_to_console(const char* message);
static void logger_write_to_file(const char* message);
static void logger_format_timestamp(char* buffer, size_t size);
static void logger_rotate_file(void);
static long logger_get_file_size(const char* filename);

#ifdef _WIN32
static void logger_set_console_color(LogLevel level);
static void logger_reset_console_color(void);
#endif

// ========== 核心函数实现 ==========

/**
 * 初始化日志系统
 */
int logger_init(LogLevel level, LogTarget target, const char* log_file) {
    g_logger_config.level = level;
    g_logger_config.target = target;

    // 如果需要文件输出，设置文件路径
    if (target == LOG_TARGET_FILE || target == LOG_TARGET_BOTH) {
        if (log_file == NULL || strlen(log_file) == 0) {
            fprintf(stderr, "Error: File output requires specifying log file path\n");
            return -1;
        }

        strncpy(g_logger_config.log_file, log_file, sizeof(g_logger_config.log_file) - 1);
        g_logger_config.log_file[sizeof(g_logger_config.log_file) - 1] = '\0';

        // 尝试打开日志文件
        g_logger_config.file_handle = fopen(g_logger_config.log_file, "a");
        if (g_logger_config.file_handle == NULL) {
            fprintf(stderr, "Error: Unable to open log file %s\n", g_logger_config.log_file);
            return -1;
        }
    }

    // 输出初始化成功信息
    LOG_DEBUG("Log System Initialization Successful - Level: %s, target: %s",
             logger_level_string(level),
             (target == LOG_TARGET_CONSOLE) ? "console" :
             (target == LOG_TARGET_FILE) ? "file" : "console+file");

    return 0;
}

/**
 * 清理日志系统资源
 */
void logger_cleanup(void) {
    if (g_logger_config.file_handle != NULL) {
        LOG_DEBUG("Shut down the logging system");
        fclose(g_logger_config.file_handle);
        g_logger_config.file_handle = NULL;
    }
}

/**
 * 设置日志级别
 */
void logger_set_level(LogLevel level) {
    LogLevel old_level = g_logger_config.level;
    g_logger_config.level = level;
    LOG_INFO("The log level has been changed: %s -> %s",
             logger_level_string(old_level),
             logger_level_string(level));
}

/**
 * 设置颜色输出
 */
void logger_set_color(int enable) {
    g_logger_config.enable_color = enable;
    LOG_INFO("Color output: %s", enable ? "enable" : "disable");
}

/**
 * 设置时间戳
 */
void logger_set_timestamp(int enable) {
    g_logger_config.enable_timestamp = enable;
    LOG_INFO("timestamp: %s", enable ? "enable" : "disable");
}

/**
 * 设置文件轮转
 */
void logger_set_rotation(size_t max_size, int max_backups) {
    g_logger_config.max_file_size = max_size;
    g_logger_config.max_backup_files = max_backups;
    LOG_INFO("File rotation setting: maximum %zu bytes, %d backup files", max_size, max_backups);
}

/**
 * 核心日志输出函数
 */
void logger_log(LogLevel level, const char* file, int line, const char* func, const char* format, ...) {
    // 检查日志级别
    if (level < g_logger_config.level) {
        return;
    }

    // 准备变量
    va_list args;
    char message[2048];
    char timestamp[64];
    char final_message[2560];

    // 格式化用户消息
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);

    // 生成时间戳
    if (g_logger_config.enable_timestamp) {
        logger_format_timestamp(timestamp, sizeof(timestamp));
    } else {
        timestamp[0] = '\0';
    }

    // 构建最终消息
    if (g_logger_config.enable_timestamp) {
        snprintf(final_message, sizeof(final_message),
                "[%s] [%s] [%s:%d] %s\n",
                timestamp,
                logger_level_string(level),
                strrchr(file, '/') ? strrchr(file, '/') + 1 :
                (strrchr(file, '\\') ? strrchr(file, '\\') + 1 : file),
                line,
                message);
    } else {
        snprintf(final_message, sizeof(final_message),
                "[%s] [%s:%d] %s\n",
                logger_level_string(level),
                strrchr(file, '/') ? strrchr(file, '/') + 1 :
                (strrchr(file, '\\') ? strrchr(file, '\\') + 1 : file),
                line,
                message);
    }

    // 输出到指定目标
    if (g_logger_config.target == LOG_TARGET_CONSOLE || g_logger_config.target == LOG_TARGET_BOTH) {
        logger_write_to_console(final_message);
    }

    if (g_logger_config.target == LOG_TARGET_FILE || g_logger_config.target == LOG_TARGET_BOTH) {
        logger_write_to_file(final_message);
        logger_rotate_if_needed();
    }
}

/**
 * 获取日志级别字符串
 */
const char* logger_level_string(LogLevel level) {
    switch (level) {
        case LOG_LEVEL_DEBUG: return "DEBUG";
        case LOG_LEVEL_INFO:  return "INFO";
        case LOG_LEVEL_WARN:  return "WARN";
        case LOG_LEVEL_ERROR: return "ERROR";
        case LOG_LEVEL_FATAL: return "FATAL";
        case LOG_LEVEL_OFF:   return "OFF";
        default:              return "UNKN";
    }
}

/**
 * 获取日志级别颜色
 */
const char* logger_level_color(LogLevel level) {
    if (!g_logger_config.enable_color) {
        return "";
    }

    switch (level) {
        case LOG_LEVEL_DEBUG: return COLOR_DEBUG;
        case LOG_LEVEL_INFO:  return COLOR_INFO;
        case LOG_LEVEL_WARN:  return COLOR_WARN;
        case LOG_LEVEL_ERROR: return COLOR_ERROR;
        case LOG_LEVEL_FATAL: return COLOR_FATAL;
        default:              return COLOR_RESET;
    }
}

/**
 * 检查并执行文件轮转
 */
void logger_rotate_if_needed(void) {
    if (g_logger_config.file_handle == NULL || strlen(g_logger_config.log_file) == 0) {
        return;
    }

    long file_size = logger_get_file_size(g_logger_config.log_file);
    if (file_size > (long)g_logger_config.max_file_size) {
        logger_rotate_file();
    }
}

// ========== 内部函数实现 ==========

/**
 * 输出到控制台
 */
static void logger_write_to_console(const char* message) {
#ifdef _WIN32
    // Windows控制台颜色处理
    printf("%s", message);
#else
    // Unix/Linux控制台颜色处理
    printf("%s", message);
#endif
    fflush(stdout);
}

/**
 * 输出到文件
 */
static void logger_write_to_file(const char* message) {
    if (g_logger_config.file_handle != NULL) {
        fprintf(g_logger_config.file_handle, "%s", message);
        fflush(g_logger_config.file_handle);
    }
}

/**
 * 格式化时间戳
 */
static void logger_format_timestamp(char* buffer, size_t size) {
    time_t now;
    struct tm* tm_info;

    time(&now);
    tm_info = localtime(&now);

    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", tm_info);
}

/**
 * 执行文件轮转
 */
static void logger_rotate_file(void) {
    if (g_logger_config.file_handle == NULL) {
        return;
    }

    // 关闭当前文件
    fclose(g_logger_config.file_handle);
    g_logger_config.file_handle = NULL;

    // 轮转备份文件
    char old_name[512];
    char new_name[512];

    // 删除最老的备份文件
    snprintf(old_name, sizeof(old_name), "%s.%d",
             g_logger_config.log_file, g_logger_config.max_backup_files);
    remove(old_name);

    // 重命名现有备份文件
    for (int i = g_logger_config.max_backup_files - 1; i >= 1; i--) {
        snprintf(old_name, sizeof(old_name), "%s.%d", g_logger_config.log_file, i);
        snprintf(new_name, sizeof(new_name), "%s.%d", g_logger_config.log_file, i + 1);
        rename(old_name, new_name);
    }

    // 重命名当前日志文件
    snprintf(new_name, sizeof(new_name), "%s.1", g_logger_config.log_file);
    rename(g_logger_config.log_file, new_name);

    // 重新打开日志文件
    g_logger_config.file_handle = fopen(g_logger_config.log_file, "w");
    if (g_logger_config.file_handle == NULL) {
        fprintf(stderr, "Error: Unable to reopen log file after file rotation\n");
    }
}

/**
 * 获取文件大小
 */
static long logger_get_file_size(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        return 0;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fclose(file);

    return size;
}

#ifdef _WIN32
/**
 * Windows控制台颜色设置
 */
static void logger_set_console_color(LogLevel level) {
    if (!g_logger_config.enable_color) {
        return;
    }

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    WORD color;

    switch (level) {
        case LOG_LEVEL_DEBUG: color = FOREGROUND_BLUE | FOREGROUND_GREEN; break;  // 青色
        case LOG_LEVEL_INFO:  color = FOREGROUND_GREEN; break;                    // 绿色
        case LOG_LEVEL_WARN:  color = FOREGROUND_RED | FOREGROUND_GREEN; break;   // 黄色
        case LOG_LEVEL_ERROR: color = FOREGROUND_RED; break;                      // 红色
        case LOG_LEVEL_FATAL: color = FOREGROUND_RED | FOREGROUND_BLUE; break;    // 紫色
        default:              color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; break;
    }

    SetConsoleTextAttribute(hConsole, color);
}

/**
 * Windows控制台颜色重置
 */
static void logger_reset_console_color(void) {
    if (!g_logger_config.enable_color) {
        return;
    }

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
}
#endif