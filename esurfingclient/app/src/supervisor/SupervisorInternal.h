#ifndef ESURFINGCLIENT_SUPERVISORINTERNAL_H
#define ESURFINGCLIENT_SUPERVISORINTERNAL_H

#include "core/States.h"

/* child_t / child_handle_t / child_kind_t / stop_flag_t / SUPERVISOR_MAX_CHILDREN 都在 States.h */

#ifdef _WIN32

#include <windows.h>

#else

#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#if defined(__linux__)
#include <sys/prctl.h>
#endif

#ifndef EINTR
#define EINTR 4
#endif

#endif

#define SUPERVISOR_HEALTHY_MS 60000

#define SUPERVISOR_BACKOFF_MIN_MS 1000
#define SUPERVISOR_BACKOFF_MAX_MS 60000

#define SUPERVISOR_STOP_WEB_MS 5000

#define SUPERVISOR_STOP_AUTH_MS 15000

#define SUPERVISOR_STOP_NO_GRACE_MS 1000

/**
 * 关窗口/注销/关机时等待子进程的上限
 *
 * 这类事件系统只给几秒, 而认证进程退出前还要发一次登出请求 (网络请求)。
 * 不能像平常那样等 15 秒 —— 等太久监管进程会先被系统杀掉, 日志就来不及归档
 * (run.log 不会被改名)。登出没做完不要紧, 下次启动会靠会话存档补上。
 */
#define SUPERVISOR_STOP_CONSOLE_MS 1200

#define SUPERVISOR_TICK_MS 200

#define SUPERVISOR_WAIT_SLICE_MS 100

const char* child_name(const child_t* child, char* buf, const size_t len);

unsigned long child_pid(const child_t* child);

bool child_spawn(child_t* child);

void child_schedule_restart(child_t* child, const int exit_code);

child_t* child_find(const child_handle_t handle);

void child_stop(child_t* child, const uint32_t grace_ms);

uint16_t child_control_port(const child_kind_t kind, const int index);

void supervisor_install_signals(void);

#endif
