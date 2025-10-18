//
// Created by bad_g on 2025/9/28.
//
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <stdlib.h>

#include "../headFiles/utils/Logger.h"

#ifdef _WIN32
    #include <windows.h>
    #include <io.h>
#else
    #include <unistd.h>
    #include <sys/stat.h>
#endif

LoggerConfig gLoggerConfig = {
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

/**
 * 获取日志级别字符串函数
 */
const char* loggerLevelString(const LogLevel level)
{
    switch (level)
    {
    case LOG_LEVEL_DEBUG: return "DEBUG";
    case LOG_LEVEL_INFO:  return "INFO";
    case LOG_LEVEL_WARN:  return "WARN";
    case LOG_LEVEL_ERROR: return "ERROR";
    case LOG_LEVEL_FATAL: return "FATAL";
    case LOG_LEVEL_OFF:   return "OFF";
    default:              return "UNKNOWN";
    }
}

/**
 * 获取文件大小函数
 */
long loggerGetFileSize(const char* filename)
{
    FILE* file = fopen(filename, "r");
    if (file == NULL)
    {
        return 0;
    }
    fseek(file, 0, SEEK_END);
    const long size = ftell(file);
    fclose(file);
    return size;
}

/**
 * 文件轮转函数
 */
void loggerRotateFile()
{
    if (gLoggerConfig.file_handle == NULL)
    {
        return;
    }
    fclose(gLoggerConfig.file_handle);
    gLoggerConfig.file_handle = NULL;
    const size_t base_len = strlen(gLoggerConfig.log_file);
    const size_t buf_size = base_len + 32;
    char* old_name = malloc(buf_size);
    char* new_name = malloc(buf_size);
    if (old_name == NULL || new_name == NULL)
    {
        fprintf(stderr, "Error: Unable to allocate rotation buffers\n");
        free(old_name);
        free(new_name);
        return;
    }
    int n = snprintf(old_name, buf_size, "%s.%d",
                     gLoggerConfig.log_file, gLoggerConfig.max_backup_files);
    if (n < 0 || (size_t)n >= buf_size)
    {
        fprintf(stderr, "Error: Rotation path too long\n");
        free(old_name);
        free(new_name);
        return;
    }
    remove(old_name);
    for (int i = gLoggerConfig.max_backup_files - 1; i >= 1; i--)
    {
        n = snprintf(old_name, buf_size, "%s.%d", gLoggerConfig.log_file, i);
        if (n < 0 || (size_t)n >= buf_size)
        {
            fprintf(stderr, "Error: Rotation path too long\n");
            break;
        }
        n = snprintf(new_name, buf_size, "%s.%d", gLoggerConfig.log_file, i + 1);
        if (n < 0 || (size_t)n >= buf_size)
        {
            fprintf(stderr, "Error: Rotation path too long\n");
            break;
        }
        rename(old_name, new_name);
    }
    n = snprintf(new_name, buf_size, "%s.1", gLoggerConfig.log_file);
    if (n < 0 || (size_t)n >= buf_size)
    {
        fprintf(stderr, "Error: Rotation path too long\n");
        free(old_name);
        free(new_name);
        return;
    }
    rename(gLoggerConfig.log_file, new_name);
    free(old_name);
    free(new_name);
    gLoggerConfig.file_handle = fopen(gLoggerConfig.log_file, "w");
    if (gLoggerConfig.file_handle == NULL)
    {
        fprintf(stderr, "Error: Unable to reopen log file after file rotation\n");
    }
}

/**
 * 检查并执行文件轮转函数
 */
void loggerRotateIfNeeded()
{
    if (gLoggerConfig.file_handle == NULL || strlen(gLoggerConfig.log_file) == 0)
    {
        return;
    }
    const long file_size = loggerGetFileSize(gLoggerConfig.log_file);
    if (file_size > (long)gLoggerConfig.max_file_size)
    {
        loggerRotateFile();
    }
}

/**
 * 获取可执行文件所在目录函数
 */
int getExecutableDir(char* out)
{
#ifdef _WIN32
    char path[MAX_PATH];
    const DWORD len = GetModuleFileNameA(NULL, path, MAX_PATH);
    if (len == 0 || len >= MAX_PATH)
    {
        return -1;
    }
    char* last = strrchr(path, '\\');
    if (!last)
    {
        return -1;
    }
    *last = '\0';
    const int n = snprintf(out, 260, "%s", path);
    if (n < 0 || (size_t)n >= 260)
    {
        return -1;
    }
    return 0;
#else
    char path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len <= 0 || len >= (ssize_t)sizeof(path))
    {
        return -1;
    }
    path[len] = '\0';
    char* last = strrchr(path, '/');
    if (!last)
    {
        return -1;
    }
    *last = '\0';
    int n = snprintf(out, 260, "%s", path);
    if (n < 0 || (size_t)n >= 260)
    {
        return -1;
    }
    return 0;
#endif
}

/**
 * 检查 log 目录函数
 */
int ensureLogDir(char* out)
{
    char dir[PATH_MAX];
    if (getExecutableDir(dir) != 0)
    {
        return -1;
    }
#ifdef _WIN32
    const int n = snprintf(out, 260, "%s\\logs", dir);
    if (n < 0 || (size_t)n >= 260)
    {
        return -1;
    }
    if (!CreateDirectoryA(out, NULL))
    {
        const DWORD err = GetLastError();
        if (err != ERROR_ALREADY_EXISTS)
        {
            return -1;
        }
    }
#else
    int n = snprintf(out, 260, "%s/log", dir);
    if (n < 0 || (size_t)n >= 260)
    {
        return -1;
    }
    struct stat st;
    if (stat(out, &st) != 0)
    {
        if (mkdir(out, 0755) != 0 && errno != EEXIST)
        {
            return -1;
        }
    }
    else if (!S_ISDIR(st.st_mode))
    {
        return -1;
    }
#endif
    return 0;
}

/**
 * 初始化日志系统函数
 */
int loggerInit(const LogLevel level, const LogTarget target)
{
    gLoggerConfig.level = level;
    gLoggerConfig.target = target;
    if (target == LOG_TARGET_FILE || target == LOG_TARGET_BOTH)
    {
        char log_dir[PATH_MAX];
        if (ensureLogDir(log_dir) != 0)
        {
            fprintf(stderr, "Error: Unable to prepare log directory\n");
            return -1;
        }
        // 使用固定的日志文件名 run.log
        const char* log_filename = "run.log";
        #ifdef _WIN32
            const char sep = '\\';
        #else
            const char sep = '/';
        #endif
        const int ln = snprintf(gLoggerConfig.log_file, sizeof(gLoggerConfig.log_file), "%s%c%s", log_dir, sep, log_filename);
        if (ln < 0 || (size_t)ln >= sizeof(gLoggerConfig.log_file))
        {
            fprintf(stderr, "Error: Log file path too long (max %zu)\n", sizeof(gLoggerConfig.log_file));
            return -1;
        }
        gLoggerConfig.file_handle = fopen(gLoggerConfig.log_file, "a");
        if (gLoggerConfig.file_handle == NULL)
        {
            fprintf(stderr, "Error: Unable to open log file %s\n", gLoggerConfig.log_file);
            return -1;
        }
    }
    LOG_DEBUG("Log System Initialization Successful - Level: %s, target: %s",
             loggerLevelString(level),
             target == LOG_TARGET_CONSOLE ? "console" :
             target == LOG_TARGET_FILE ? "file" : "console+file");
    return 0;
}

/**
 * 清理日志系统函数
 */
void loggerCleanup()
{
    if (gLoggerConfig.file_handle != NULL)
    {
        LOG_DEBUG("Shut down the logging system");
        fclose(gLoggerConfig.file_handle);
        gLoggerConfig.file_handle = NULL;
        if (strlen(gLoggerConfig.log_file) > 0)
        {
            time_t now;
            time(&now);
            char new_filename[PATH_MAX];
            char log_dir[PATH_MAX];
            strcpy(log_dir, gLoggerConfig.log_file);
            char* last_sep = strrchr(log_dir,
                #ifdef _WIN32
                    '\\'
                #else
                    '/'
                #endif
            );
            if (last_sep != NULL)
            {
                *last_sep = '\0';  // 截断到目录路径
            }
            #ifdef _WIN32
            struct tm tm_info;
            if (localtime_s(&tm_info, &now) == 0)
            {
                char time_name[64];
                strftime(time_name, sizeof(time_name), "%Y%m%d_%H%M%S.log", &tm_info);
                snprintf(new_filename, sizeof(new_filename), "%s\\%s", log_dir, time_name);
                if (rename(gLoggerConfig.log_file, new_filename) == 0)
                {
                    printf("Log file renamed to: %s\n", new_filename);
                }
                else
                {
                    fprintf(stderr, "Warning: Failed to rename log file from %s to %s\n",
                           gLoggerConfig.log_file, new_filename);
                }
            }
            #else
            struct tm* tm_info = localtime(&now);
            if (tm_info != NULL)
            {
                char time_name[64];
                strftime(time_name, sizeof(time_name), "%Y%m%d_%H%M%S.log", tm_info);
                snprintf(new_filename, sizeof(new_filename), "%s/%s", log_dir, time_name);
                if (rename(gLoggerConfig.log_file, new_filename) == 0)
                {
                    printf("Log file renamed to: %s\n", new_filename);
                }
                else
                {
                    fprintf(stderr, "Warning: Failed to rename log file from %s to %s\n",
                           gLoggerConfig.log_file, new_filename);
                }
            }
            #endif
        }
    }
}

/**
 * 设置文件轮转函数
 */
void loggerSetRotation(const size_t max_size, const int max_backups)
{
    gLoggerConfig.max_file_size = max_size;
    gLoggerConfig.max_backup_files = max_backups;
    LOG_INFO("File rotation setting: maximum %zu bytes, %d backup files", max_size, max_backups);
}

/**
 * 格式化时间戳函数
 */
void loggerFormatTimestamp(char* buffer)
{
    time_t now;
    time(&now);
    const struct tm* tm_info = localtime(&now);
    strftime(buffer, 64, "%Y-%m-%d %H:%M:%S", tm_info);
}

/**
 * 输出到控制台函数
 */
void loggerWriteToConsole(const char* message)
{
#ifdef _WIN32
    printf("%s", message);
#else
    printf("%s", message);
#endif
    fflush(stdout);
}

/**
 * 输出到文件函数
 */
void loggerWriteToFile(const char* message)
{
    if (gLoggerConfig.file_handle != NULL)
    {
        fprintf(gLoggerConfig.file_handle, "%s", message);
        fflush(gLoggerConfig.file_handle);
    }
}

/**
 * 核心日志输出函数
 */
void loggerLog(const LogLevel level, const char* file, const int line, const char* format, ...)
{
    if (level < gLoggerConfig.level)
    {
        return;
    }
    va_list args;
    char message[2048];
    char timestamp[64];
    char finalMessage[2560];
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    if (gLoggerConfig.enable_timestamp)
    {
        loggerFormatTimestamp(timestamp);
    }
    else
    {
        timestamp[0] = '\0';
    }
    if (gLoggerConfig.enable_timestamp)
    {
        snprintf(finalMessage, sizeof(finalMessage),
                "[%s] [%s] [%s:%d] %s\n",
                timestamp,
                loggerLevelString(level),
                strrchr(file, '/') ? strrchr(file, '/') + 1 :
                (strrchr(file, '\\') ? strrchr(file, '\\') + 1 : file),
                line,
                message);
    }
    else
    {
        snprintf(finalMessage, sizeof(finalMessage),
                "[%s] [%s:%d] %s\n",
                loggerLevelString(level),
                strrchr(file, '/') ? strrchr(file, '/') + 1 :
                (strrchr(file, '\\') ? strrchr(file, '\\') + 1 : file),
                line,
                message);
    }
    if (gLoggerConfig.target == LOG_TARGET_CONSOLE || gLoggerConfig.target == LOG_TARGET_BOTH)
    {
        loggerWriteToConsole(finalMessage);
    }
    if (gLoggerConfig.target == LOG_TARGET_FILE || gLoggerConfig.target == LOG_TARGET_BOTH)
    {
        loggerWriteToFile(finalMessage);
        loggerRotateIfNeeded();
    }
}