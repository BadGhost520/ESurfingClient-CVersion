#ifndef ESURFINGCLIENT_LOGGER_H
#define ESURFINGCLIENT_LOGGER_H

#include <stdbool.h>
#include <stdio.h>

#ifndef _WIN32

#include <sys/sendfile.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#endif

#ifndef PATH_MAX
#define PATH_MAX 260
#endif

typedef enum {
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
    size_t      cur_lines;
} LoggerConfig;

typedef struct {
    char* data;
    size_t size;
    bool is_new;
} LogContent;

#define LOG_VERBOSE(fmt, ...) \
log_out(LOG_LEVEL_VERBOSE, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define LOG_DEBUG(fmt, ...) \
log_out(LOG_LEVEL_DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define LOG_INFO(fmt, ...) \
log_out(LOG_LEVEL_INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define LOG_WARN(fmt, ...) \
log_out(LOG_LEVEL_WARN, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define LOG_ERROR(fmt, ...) \
log_out(LOG_LEVEL_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define LOG_FATAL(fmt, ...) \
log_out(LOG_LEVEL_FATAL, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define LOG_WEB_VERBOSE(file, line, fmt, ...) \
log_out(LOG_LEVEL_VERBOSE, file, line, fmt, ##__VA_ARGS__)

#define LOG_WEB_INFO(file, line, fmt, ...) \
log_out(LOG_LEVEL_INFO, file, line, fmt, ##__VA_ARGS__)

#define LOG_WEB_ERROR(file, line, fmt, ...) \
log_out(LOG_LEVEL_ERROR, file, line, fmt, ##__VA_ARGS__)

/**
 * @brief 打印日志
 * @param level 日志等级
 * @param file 调用的源代码文件名
 * @param line 执行该函数的行数
 * @param fmt 格式
 * @param ... 其它参数
 */
void log_out(LogLevel level, const char* file, uint32_t line, const char* fmt, ...);

/**
 * @brief 获取当前日志等级
 * @return 日志等级
 */
LogLevel get_logger_level();

/**
 * @brief 设置日志等级
 * @param level 日志等级
 */
void set_logger_level(LogLevel level);

/**
 * @brief 初始化日志系统
 * @return 初始化状态
 */
LoggerInitStatus init_logger();

/**
 * @brief 清理日志系统
 */
void clean_logger();

#endif //ESURFINGCLIENT_LOGGER_H
