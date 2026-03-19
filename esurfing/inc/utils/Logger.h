#ifndef ESURFINGCLIENT_LOGGER_H
#define ESURFINGCLIENT_LOGGER_H

#include <stdbool.h>
#include <stdio.h>

typedef enum {
    LOG_LEVEL_NONE = 0,
    LOG_LEVEL_FATAL = 1,
    LOG_LEVEL_ERROR = 2,
    LOG_LEVEL_WARN  = 3,
    LOG_LEVEL_INFO  = 4,
    LOG_LEVEL_DEBUG = 5,
    LOG_LEVEL_VERBOSE = 6
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
    bool is_new;
} LogContent;

#define LOG_VERBOSE(format, ...) \
loggerLog(LOG_LEVEL_VERBOSE, __FILE__, __LINE__, format, ##__VA_ARGS__)

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

#define LOG_WEB_VERBOSE(file, line, format, ...) \
loggerLog(LOG_LEVEL_VERBOSE, file, line, format, ##__VA_ARGS__)

#define LOG_WEB_INFO(file, line, format, ...) \
loggerLog(LOG_LEVEL_INFO, file, line, format, ##__VA_ARGS__)

#define LOG_WEB_ERROR(file, line, format, ...) \
loggerLog(LOG_LEVEL_ERROR, file, line, format, ##__VA_ARGS__)

/**
 * 打印日志
 * @param level 日志等级
 * @param file 调用的源代码文件名
 * @param line 执行该函数的行数
 * @param format 格式
 * @param ... 其它参数
 */
void loggerLog(LogLevel level, const char* file, int line, const char* format, ...);

/**
 * 获取当前日志等级
 * @return 日志等级
 */
LogLevel getLoggerLevel();

/**
 * 初始化日志系统
 * @param logger_level 日志等级
 * @return 初始化状态
 */
LoggerInitStatus loggerInit(LogLevel logger_level);

/**
 * 清理日志系统
 */
void loggerCleanup();

/**
 * 获取日志
 * @return 日志内容
 */
LogContent getLog(bool check);

#endif //ESURFINGCLIENT_LOGGER_H