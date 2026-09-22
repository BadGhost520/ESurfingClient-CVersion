#ifndef ESURFINGCLIENT_SUPERVISORINTERNAL_H
#define ESURFINGCLIENT_SUPERVISORINTERNAL_H

#include "states/States.h"

#ifdef _WIN32

#include <windows.h>

typedef HANDLE child_handle_t;
#define CHILD_HANDLE_INVALID NULL
typedef volatile LONG stop_flag_t;

#else

#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#if defined(__linux__)
#include <sys/prctl.h>
#endif

typedef pid_t child_handle_t;
#define CHILD_HANDLE_INVALID ((pid_t)-1)
typedef volatile sig_atomic_t stop_flag_t;

#ifndef EINTR
#define EINTR 4
#endif

#endif

#define SUPERVISOR_MAX_CHILDREN 17

#define SUPERVISOR_HEALTHY_MS 60000

#define SUPERVISOR_BACKOFF_MIN_MS 1000
#define SUPERVISOR_BACKOFF_MAX_MS 60000

#define SUPERVISOR_STOP_WEB_MS 5000

#define SUPERVISOR_STOP_AUTH_MS 15000

#define SUPERVISOR_STOP_NO_GRACE_MS 1000

#define SUPERVISOR_TICK_MS 200

#define SUPERVISOR_WAIT_SLICE_MS 100

typedef enum
{
    CHILD_WEB = 0,
    CHILD_AUTH = 1
} child_kind_t;

typedef struct
{
    child_handle_t handle;
    child_kind_t kind;
    uint8_t account;
    uint16_t control_port;
    bool running;
    uint32_t restarts;
    uint64_t start_time;
    uint64_t next_start_time;
} child_t;

extern child_t s_children[SUPERVISOR_MAX_CHILDREN];
extern int s_child_count;
extern stop_flag_t s_stop_requested;

const char* child_name(const child_t* child, char* buf, const size_t len);

unsigned long child_pid(const child_t* child);

bool child_spawn(child_t* child);

void child_schedule_restart(child_t* child, const int exit_code);

child_t* child_find(const child_handle_t handle);

void child_stop(child_t* child, const uint32_t grace_ms);

uint16_t child_control_port(const child_kind_t kind, const int index);

void supervisor_install_signals();

#endif
