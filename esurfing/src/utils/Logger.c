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
#include <errno.h>
#include <limits.h>

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
static int get_executable_dir(char* out, size_t size);
static int ensure_log_dir(char* out, size_t size);

// ========== 核心函数实现 ==========

/**
 * 初始化日志系统
 */
int logger_init(LogLevel level, LogTarget target) {
    g_logger_config.level = level;
    g_logger_config.target = target;

    // 如果需要文件输出，设置文件路径
    if (target == LOG_TARGET_FILE || target == LOG_TARGET_BOTH) {
        // 计算可执行文件所在目录，并在其中创建 log 目录
        char log_dir[PATH_MAX];
        if (ensure_log_dir(log_dir, sizeof(log_dir)) != 0) {
            fprintf(stderr, "Error: Unable to prepare log directory\n");
            return -1;
        }
        // 生成以当前时间命名的日志文件名（例如：20251012_153045.log）
        time_t now;
        time(&now);
#ifdef _WIN32
        struct tm tm_info;
        if (localtime_s(&tm_info, &now) != 0) {
            fprintf(stderr, "Error: Unable to get localtime for log filename\n");
            return -1;
        }
        char time_name[64];
        strftime(time_name, sizeof(time_name), "%Y%m%d_%H%M%S.log", &tm_info);
#else
        struct tm* tm_info = localtime(&now);
        if (tm_info == NULL) {
            fprintf(stderr, "Error: Unable to get localtime for log filename\n");
            return -1;
        }
        char time_name[64];
        strftime(time_name, sizeof(time_name), "%Y%m%d_%H%M%S.log", tm_info);
#endif

        // 组合最终日志文件路径
#ifdef _WIN32
        const char sep = '\\';
#else
        const char sep = '/';
#endif
        int ln = snprintf(g_logger_config.log_file, sizeof(g_logger_config.log_file), "%s%c%s", log_dir, sep, time_name);
        if (ln < 0 || (size_t)ln >= sizeof(g_logger_config.log_file)) {
            fprintf(stderr, "Error: Log file path too long (max %zu)\n", sizeof(g_logger_config.log_file));
            return -1;
        }

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

    // 轮转备份文件（使用动态缓冲避免路径截断）
    size_t base_len = strlen(g_logger_config.log_file);
    size_t buf_size = base_len + 32; // 足够容纳诸如 ".123456789" 的后缀
    char* old_name = (char*)malloc(buf_size);
    char* new_name = (char*)malloc(buf_size);
    if (old_name == NULL || new_name == NULL) {
        fprintf(stderr, "Error: Unable to allocate rotation buffers\n");
        free(old_name);
        free(new_name);
        return;
    }

    // 删除最老的备份文件
    int n = snprintf(old_name, buf_size, "%s.%d",
                     g_logger_config.log_file, g_logger_config.max_backup_files);
    if (n < 0 || (size_t)n >= buf_size) {
        fprintf(stderr, "Error: Rotation path too long\n");
        free(old_name);
        free(new_name);
        return;
    }
    remove(old_name);

    // 重命名现有备份文件
    for (int i = g_logger_config.max_backup_files - 1; i >= 1; i--) {
        n = snprintf(old_name, buf_size, "%s.%d", g_logger_config.log_file, i);
        if (n < 0 || (size_t)n >= buf_size) {
            fprintf(stderr, "Error: Rotation path too long\n");
            break;
        }
        n = snprintf(new_name, buf_size, "%s.%d", g_logger_config.log_file, i + 1);
        if (n < 0 || (size_t)n >= buf_size) {
            fprintf(stderr, "Error: Rotation path too long\n");
            break;
        }
        rename(old_name, new_name);
    }

    // 重命名当前日志文件
    n = snprintf(new_name, buf_size, "%s.1", g_logger_config.log_file);
    if (n < 0 || (size_t)n >= buf_size) {
        fprintf(stderr, "Error: Rotation path too long\n");
        free(old_name);
        free(new_name);
        return;
    }
    rename(g_logger_config.log_file, new_name);

    free(old_name);
    free(new_name);

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

/**
 * 获取可执行文件所在目录
 * 返回0成功，非0失败
 */
static int get_executable_dir(char* out, size_t size) {
#ifdef _WIN32
    char path[MAX_PATH];
    DWORD len = GetModuleFileNameA(NULL, path, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) {
        return -1;
    }
    // 寻找最后一个路径分隔符
    char* last = strrchr(path, '\\');
    if (!last) {
        return -1;
    }
    *last = '\0'; // 截断到目录
    int n = snprintf(out, size, "%s", path);
    if (n < 0 || (size_t)n >= size) {
        return -1;
    }
    return 0;
#else
    char path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len <= 0 || len >= (ssize_t)sizeof(path)) {
        return -1;
    }
    path[len] = '\0';
    // 寻找最后一个路径分隔符
    char* last = strrchr(path, '/');
    if (!last) {
        return -1;
    }
    *last = '\0'; // 截断到目录
    int n = snprintf(out, size, "%s", path);
    if (n < 0 || (size_t)n >= size) {
        return -1;
    }
    return 0;
#endif
}

/**
 * 确保程序目录下的 log 目录存在；返回 log 目录路径
 */
static int ensure_log_dir(char* out, size_t size) {
    char dir[PATH_MAX];
    if (get_executable_dir(dir, sizeof(dir)) != 0) {
        return -1;
    }
#ifdef _WIN32
    int n = snprintf(out, size, "%s\\log", dir);
    if (n < 0 || (size_t)n >= size) {
        return -1;
    }
    if (!CreateDirectoryA(out, NULL)) {
        DWORD err = GetLastError();
        if (err != ERROR_ALREADY_EXISTS) {
            return -1;
        }
    }
#else
    int n = snprintf(out, size, "%s/log", dir);
    if (n < 0 || (size_t)n >= size) {
        return -1;
    }
    struct stat st;
    if (stat(out, &st) != 0) {
        if (mkdir(out, 0755) != 0 && errno != EEXIST) {
            return -1;
        }
    } else if (!S_ISDIR(st.st_mode)) {
        // 已存在但不是目录
        return -1;
    }
#endif
    return 0;
}