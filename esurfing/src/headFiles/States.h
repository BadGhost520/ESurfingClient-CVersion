#ifndef ESURFINGCLIENT_STATES_H
#define ESURFINGCLIENT_STATES_H

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
    bool is_initialized;
    bool is_connected;
    bool is_running;
    bool is_logged;
} RuntimeStatus;

typedef struct
{
    int64_t connect_time;
    int64_t auth_time;
} Timestamp;

typedef struct
{
    pthread_t thread;
    int status;
} ThreadStatus;

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
    Timestamp timestamp;
    Options options;
    int index;
} DialerContext;

extern __thread DialerContext dialer_adapter;
extern ThreadStatus thread_status[MAX_DIALER_COUNT];
extern bool adapter_need_stop[MAX_DIALER_COUNT];

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