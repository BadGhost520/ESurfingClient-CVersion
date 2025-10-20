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
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO  = 1,
    LOG_LEVEL_WARN  = 2,
    LOG_LEVEL_ERROR = 3,
    LOG_LEVEL_FATAL = 4,
    LOG_LEVEL_OFF   = 5
} LogLevel;

typedef enum {
    LOG_TARGET_CONSOLE = 1,
    LOG_TARGET_FILE    = 2,
    LOG_TARGET_BOTH    = 3
} LogTarget;

typedef struct {
    LogLevel    level;
    char        logDir[PATH_MAX];
    char        logFile[PATH_MAX];
    FILE*       fileHandle;
    int         maxBackupFiles;
    size_t      maxLines;
    size_t      currentLines;
} LoggerConfig;

/**
 * 初始化日志系统函数
 */
int loggerInit(LogLevel level);

/**
 * 清理日志系统函数
 */
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