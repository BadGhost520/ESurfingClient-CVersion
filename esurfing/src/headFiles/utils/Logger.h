#ifndef ESURFINGCLIENT_LOGGER_H
#define ESURFINGCLIENT_LOGGER_H

#include <stdbool.h>
#include <stdio.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO  = 1,
    LOG_LEVEL_WARN  = 2,
    LOG_LEVEL_ERROR = 3,
    LOG_LEVEL_FATAL = 4
} LogLevel;

typedef enum
{
    INIT_LOGGER_FAILURE = 0,
    INIT_LOGGER_SUCCESS = 1,
} LoggerInitStatus;

typedef struct {
    LogLevel    level;
    char        log_dir[PATH_MAX];
    char        log_file[PATH_MAX];
    FILE*       file_handle;
    size_t      max_lines;
    size_t      current_lines;
} LoggerConfig;

typedef struct {
    char* data;
    size_t size;
} LogContent;

typedef struct
{
    bool is_debug;
    bool is_small_device;
} LoggerSettings;

#define LOG_DEBUG(format, ...) \
loggerLog(LOG_LEVEL_DEBUG, __FILE__, __LINE__, format, ##__VA_ARGS__)

#define LOG_INFO(format, ...) \
loggerLog(LOG_LEVEL_INFO, __FILE__, __LINE__, format, ##__VA_ARGS__)

#define LOG_WARN(format, ...) \
loggerLog(LOG_LEVEL_WARN, __FILE__, __LINE__, format, ##__VA_ARGS__)

#define LOG_ERROR(format, ...) \
loggerLog(LOG_LEVEL_ERROR, __FILE__, __LINE__, format, ##__VA_ARGS__)

#define LOG_FATAL(format, ...) \
loggerLog(LOG_LEVEL_FATAL, __FILE__, __LINE__, format, ##__VA_ARGS__)

/**
 * 打印日志
 * @param level 日志等级
 * @param file 日志文件路径
 * @param line 行数
 * @param format 格式
 * @param ... 其它参数
 */
void loggerLog(LogLevel level, const char* file, int line, const char* format, ...);

/**
 * 初始化日志系统
 * @param logger_settings 日志设置
 * @return 初始化状态
 */
LoggerInitStatus loggerInit(LoggerSettings logger_settings);

/**
 * 清理日志系统
 */
void loggerCleanup();

/**
 * 获取内存日志
 * @return 增量内存日志
 */
LogContent getLog();

#endif //ESURFINGCLIENT_LOGGER_H