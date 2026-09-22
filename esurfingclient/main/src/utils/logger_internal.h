#ifndef ESURFINGCLIENT_LOGGER_INTERNAL_H
#define ESURFINGCLIENT_LOGGER_INTERNAL_H

#include "utils/PlatformUtils.h"
#include "utils/Logger.h"

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include <pthread.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#ifndef EEXIST
#define EEXIST 17
#endif
#endif

extern const char s_file_name[];
extern char s_cfg_log_dir[PATH_MAX];
extern bool s_logger_inited;
extern log_cfg_t s_logger_cfg;
extern bool s_console_enabled;
extern bool s_query_mode;

/** @brief 日志等级名 (log_core.c) */
const char* get_level_str(const LogLevel lv);

/** @brief 日志互斥 (log_core.c, 平台分支各一份实现) */
void logger_lock(void);

void logger_unlock(void);

/** @brief 组装日志里的"是谁写的" / "是哪个线程"那两段 (log_meta.c) */
void get_proc_str(char* buf, const size_t len);

void get_thread_str(char* buf, const size_t len);

/** @brief 日志文件句柄的打开与轮转 (log_file.c) */
bool open_log_file();

void rotate();

/** @brief 取实际使用的日志目录 (logger_dir.c) */
bool get_log_dir(char* out);

#endif
