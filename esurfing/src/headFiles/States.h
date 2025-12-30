#ifndef ESURFINGCLIENT_STATES_H
#define ESURFINGCLIENT_STATES_H

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

#define MAX_DIALER_COUNT 2

typedef struct
{
    char* mac_address;
    char* ticket_url;
    char* client_id;
    char* school_id;
    char* auth_url;
    char* algo_id;
    char* user_ip;
    char* domain;
    char* ticket;
    char* ac_ip;
    char* area;
} AuthConfig;

typedef struct
{
    bool is_settings_changed;
    bool is_initialized;
    bool is_running;
    bool is_authed;
} RuntimeStatus;

typedef struct
{
    char* usr;
    char* pwd;
    char* chn;
} Options;

typedef struct
{
    AuthConfig auth_config;
    RuntimeStatus runtime_status;
    int64_t auth_time;
    Options options;
} DialerContext;

typedef struct
{
    DialerContext dialer_context;
    pthread_t thread;
    int thread_status;
    bool thread_is_running;
    bool need_stop;
} ThreadStatus;

typedef struct
{
    bool is_connected;
    int64_t connect_time;
} ConnectionStatus;

extern __thread int thread_index;
extern ThreadStatus thread_status[MAX_DIALER_COUNT];
extern int64_t g_running_time;

/**
 * 刷新状态函数
 */
void refreshStates();

/**
 * 设置适配器配置选项
 * @param opt 选项
 */
void setOpt(Options opt);

#endif //ESURFINGCLIENT_STATES_H