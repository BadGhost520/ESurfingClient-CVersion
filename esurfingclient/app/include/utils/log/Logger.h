#ifndef ESURFINGCLIENT_LOGGER_H
#define ESURFINGCLIENT_LOGGER_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/**
 * 兜底: 有些模块 (Config.c / LogoutState.c 等) 通过这个头拿到 PATH_MAX 就不另
 * include <limits.h> 了。日志自己的结构体不再用它, 但为了不打断这些调用方保留。
 */
#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

/**
 * 日志模块自己的路径长度上限
 *
 * 不能用 PATH_MAX: 它由 <limits.h> 定义、Windows 上是 260、其它平台是 1024,
 * 而 log_cfg_t 是跨编译单元的结构体 —— 一个 TU 见过 limits.h、另一个没见过,
 * 结构体尺寸就不一致了。固定成项目自己的常量才能保证布局稳定。
 */
#define LOG_PATH_MAX 1024

#define DEFAULT_LOG_DIR "./"

typedef enum {
    LOG_LEVEL_NONE = 0,
    LOG_LEVEL_FATAL = 1,
    LOG_LEVEL_ERROR = 2,
    LOG_LEVEL_WARN  = 3,
    LOG_LEVEL_INFO  = 4,
    LOG_LEVEL_DEBUG = 5,
    LOG_LEVEL_VERBOSE = 6
} log_level_t;

typedef struct {
    log_level_t    lv;
    char        log_dir[LOG_PATH_MAX];
    char        log_file[LOG_PATH_MAX];
    FILE*       file_handle;
    size_t      max_lines;
    size_t      cur_lines;
    uint64_t    file_dev;
    uint64_t    file_ino;
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
void log_out(log_level_t level, const char* file, uint32_t line, const char* fmt, ...);

/**
 * @brief 不走常规日志流程, 直接往日志文件写一行 (给看门狗用)
 *
 * 判定卡死时主线程可能正卡在某个调用里, 这里只做一次 write: 不判轮转、不建目录。
 * 格式与常规日志一致, 解析工具不用开特例。
 * @param text 要写出的内容 (会自动补换行)
 */
void log_raw_line(const char* text);

/**
 * @brief 获取当前日志等级
 * @return 日志等级
 */
log_level_t get_logger_level(void);

/**
 * @brief 设置日志等级
 * @param lv 日志等级
 */
void set_logger_level(log_level_t lv);

/**
 * @brief 按配置里的 log_dir 设置日志目录
 *
 * log_dir 是【基目录】, 日志放在它下面的 logs 里; 相对路径按程序所在目录解析。
 * init_logger() 之后调用会连已打开的日志一起搬过去 (启动那几行是在读配置之前写的),
 * 之前调用则只记下目录。
 * @param dir 配置里的基目录 (空字符串表示保持默认目录)
 * @return 是否设置成功 (失败时仍旧使用默认目录)
 */
bool set_logger_dir(const char* dir);

/**
 * @brief 初始化日志系统
 * @return 初始化状态
 */
bool init_logger(void);

/**
 * @brief 清理日志系统
 */
void clean_logger(void);

/**
 * @brief 获取实际使用的日志目录 (未初始化时返回空字符串)
 * @return 日志目录
 */
const char* get_logger_dir(void);

/**
 * @brief 取配置里写的日志基目录 (原文, 未配置时返回默认值)
 *
 * 给页面回显与写回配置用; 实际在用的目录 (基目录下的 logs) 走 get_logger_dir()
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

/**
 * @brief 设置查询模式 (日志一行都不落盘, 全部改写到 stderr)
 *
 * 给 --print-log-dir / --list-accounts 这类查询用: 它们要 load_cfg() 才能得到答案,
 * 而解析配置会写日志 —— 落进 run.log 就会被启动脚本当成上一轮日志归档走。
 * 改写到 stderr 而不是彻底不写, 是为了配置有问题时仍能看见原因。
 * @param enabled 是否查询模式
 */
void set_logger_query_mode(bool enabled);

/**
 * @brief 一次性查询实际使用的日志目录 (供 --print-log-dir 使用)
 *
 * 目录由配置决定, 所以这里会完整跑一遍 init_logger + load_cfg; 给 OpenWrt 的
 * init.d 归档日志、LuCI 日志页列表用。
 * @return 日志目录 (进程内静态缓冲, 不要 free); 配置有问题或日志起不来时返回 NULL
 */
const char* print_log_dir(void);

#endif
