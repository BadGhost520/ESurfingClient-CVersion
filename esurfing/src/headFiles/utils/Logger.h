//
// Created by bad_g on 2025/9/28.
//

#ifndef ESURFINGCLIENT_LOGGER_H
#define ESURFINGCLIENT_LOGGER_H

#include <stdio.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

typedef enum {
    LOG_LEVEL_DEBUG = 0,    // 调试信息
    LOG_LEVEL_INFO  = 1,    // 一般信息
    LOG_LEVEL_WARN  = 2,    // 警告信息
    LOG_LEVEL_ERROR = 3,    // 错误信息
    LOG_LEVEL_FATAL = 4,    // 致命错误
    LOG_LEVEL_OFF   = 5     // 关闭日志
} LogLevel;

typedef enum {
    LOG_TARGET_CONSOLE = 1,     // 控制台输出
    LOG_TARGET_FILE    = 2,     // 文件输出
    LOG_TARGET_BOTH    = 3      // 同时输出到控制台和文件
} LogTarget;

typedef struct {
    LogLevel    level;
    char        logFile[PATH_MAX];
    FILE*       fileHandle;
    size_t      maxFileSize;
    int         maxBackupFiles;
} LoggerConfig;

extern LoggerConfig gLoggerConfig;

int loggerInit(LogLevel level);

void loggerCleanup();

void loggerLog(LogLevel level, const char* file, int line, const char* format, ...);

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

#endif //ESURFINGCLIENT_LOGGER_H