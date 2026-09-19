#ifndef ESURFINGCLIENT_LOGGER_H
#define ESURFINGCLIENT_LOGGER_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#ifndef PATH_MAX
#define PATH_MAX 260
#endif

/**
 * @brief 配置文件里没写 log_dir 时用的默认值
 *
 * 与 config/ESurfingClient.json 里的一致: 基目录就是程序所在目录,
 * 日志放在它下面的 logs 里 (补全配置时也要用这个值)
 */
#define DEFAULT_LOG_DIR "./"

typedef enum {
    LOG_LEVEL_NONE = 0,
    LOG_LEVEL_FATAL = 1,
    LOG_LEVEL_ERROR = 2,
    LOG_LEVEL_WARN  = 3,
    LOG_LEVEL_INFO  = 4,
    LOG_LEVEL_DEBUG = 5,
    LOG_LEVEL_VERBOSE = 6
} LogLevel;

typedef struct {
    LogLevel    lv;
    char        log_dir[PATH_MAX];
    char        log_file[PATH_MAX];
    FILE*       file_handle;
    size_t      max_lines;
    size_t      cur_lines;
    /** @brief 当前持有的日志文件设备号 (Windows 为卷序列号) */
    uint64_t    file_dev;
    /** @brief 当前持有的日志文件 inode (Windows 为文件索引) */
    uint64_t    file_ino;
    /** @brief 距上次文件身份复检已写入的行数 */
    size_t      lines_since_check;
} log_cfg_t;

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
 * @brief 不走常规日志流程, 直接往日志文件写一行
 *
 * 给看门狗用: 判定卡死时主线程可能正卡在某个调用里, 常规的 log_out 会去
 * 判断轮转、可能要建目录, 都不该在那种状态下做。这里只做一次 write。
 * 格式与常规日志一致, 解析日志的工具不用为它开特例。
 * @param text 要写出的内容 (会自动补换行)
 */
void log_raw_line(const char* text);

/**
 * @brief 获取当前日志等级
 * @return 日志等级
 */
LogLevel get_logger_level();

/**
 * @brief 设置日志等级
 * @param lv 日志等级
 */
void set_logger_level(LogLevel lv);

/**
 * @brief 按配置文件里的 log_dir 设置日志目录
 *
 * 通常是在 init_logger() 之后调用 (配置文件就是那时候才读的): 那时不只是记下目录,
 * 还得把已经打开的那份日志搬过去 —— 启动的那几行是在配置加载之前写下的,
 * 留在默认目录里就没人收尾了 (见实现里的说明)。
 * 在 init_logger() 之前调用也允许, 那时只记下目录, 等 init 时生效。
 *
 * log_dir 是【基目录】: 日志放在它下面的 logs 里 (OpenWrt 上就是
 * /var/log/esurfing/logs)。相对路径按【程序所在目录】解析;
 * OpenWrt 上基目录写死, 本函数不生效。
 * @param dir 配置里的基目录 (空字符串表示保持默认目录)
 * @return 是否设置成功 (失败时仍旧使用默认目录)
 */
bool set_logger_dir(const char* dir);

/**
 * @brief 初始化日志系统
 * @return 初始化状态
 */
bool init_logger();

/**
 * @brief 清理日志系统
 */
void clean_logger();

/**
 * @brief 获取实际使用的日志目录 (未初始化时返回空字符串)
 * @return 日志目录
 */
const char* get_logger_dir(void);

/**
 * @brief 获取配置文件里写的日志基目录
 *
 * 与 get_logger_dir() 的区别: 这个返回的是配置里的原文 (没写时回默认值 "./",
 * 也就是程序所在目录), 给页面回显与写回配置用; 那个返回的是实际在用的目录
 * (基目录下的 logs), 给查看日志用
 * @return 配置里的日志基目录
 */
const char* get_logger_dir_cfg(void);

/**
 * @brief 设置是否同时把日志输出到控制台
 *
 * 列举账号时 stdout 要留给账号列表, 不能被日志内容污染
 * @param enabled 是否输出到控制台
 */
void set_logger_console(bool enabled);

#endif //ESURFINGCLIENT_LOGGER_H
