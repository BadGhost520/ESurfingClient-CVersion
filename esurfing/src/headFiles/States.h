#ifndef ESURFINGCLIENT_STATES_H
#define ESURFINGCLIENT_STATES_H

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

#define MAX_DIALER_COUNT 2

#define MAC_ADDRESS_LENGTH 20
#define TICKET_URL_LENGTH 256
#define USER_AGENT_LENGTH 32
#define CLIENT_ID_LENGTH 40
#define HOST_NAME_LENGTH 16
#define AUTH_URL_LENGTH 80
#define ALGO_ID_LENGTH 37
#define TICKET_LENGTH 40

#define USR_LENGTH 16
#define PWD_LENGTH 128
#define CHN_LENGTH 8

#define IP_LENGTH 16
#define ADAPTER_NAME_LENGTH 128

typedef struct
{
    char mac_address[MAC_ADDRESS_LENGTH];
    char ticket_url[TICKET_URL_LENGTH];
    char user_agent[USER_AGENT_LENGTH];
    char client_id[CLIENT_ID_LENGTH];
    char host_name[HOST_NAME_LENGTH];
    char auth_url[AUTH_URL_LENGTH];
    char algo_id[ALGO_ID_LENGTH];
    char ticket[TICKET_LENGTH];
    char client_ip[IP_LENGTH];
    char ac_ip[IP_LENGTH];
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
    char usr[USR_LENGTH];
    char pwd[PWD_LENGTH];
    char chn[CHN_LENGTH];
    bool auto_start;
} Options;

typedef struct
{
    RuntimeStatus runtime_status;
    AuthConfig auth_config;
    int64_t auth_time;
    Options options;
} DialerContext;

typedef struct
{
    DialerContext dialer_context;
    int thread_status;
    pthread_t thread;
    bool thread_is_running;
    bool need_stop;
} ThreadStatus;

typedef struct
{
    bool is_connected;
    int64_t connect_time;
} ConnectionStatus;

typedef struct
{
    char name[ADAPTER_NAME_LENGTH];
    char ip[IP_LENGTH];
} Adapters;

typedef struct
{
    bool is_used;
    char ip[IP_LENGTH];
} SchoolConnectionStatus;

extern SchoolConnectionStatus school_connection_status[MAX_DIALER_COUNT];
extern ThreadStatus thread_status[MAX_DIALER_COUNT];
extern int64_t g_running_time;
extern Adapters adaptor[16];

/**
 * 刷新状态函数
 */
void refreshStates();

/**
 * 设置适配器配置选项
 * @param opt 选项
 * @param index 需要修改的适配器下标
 */
void setOpt(Options opt, int index);

#endif //ESURFINGCLIENT_STATES_H