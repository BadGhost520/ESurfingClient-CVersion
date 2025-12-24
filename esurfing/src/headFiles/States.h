#ifndef ESURFINGCLIENT_STATES_H
#define ESURFINGCLIENT_STATES_H

#include <stdint.h>

typedef enum
{
    ADAPTER_1 = 1,
    ADAPTER_2 = 2,
} Adapter;

typedef struct
{
    Adapter adapter;
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
    int is_webserver_running;
    int is_settings_change;
    int is_initialized;
    int is_connected;
    int is_running;
    int is_logged;
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
    AuthConfig auth_config;
    RuntimeStatus status;
    Timestamp timestamp;
    ThreadStatus thread;
} DialerContext;

extern DialerContext dialer_adapter_1;
extern DialerContext dialer_adapter_2;

/**
 * 刷新状态函数
 */
void refreshStates();

#endif //ESURFINGCLIENT_STATES_H