#ifndef ESURFINGCLIENT_STATES_H
#define ESURFINGCLIENT_STATES_H

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

#define MAX_DIALER_COUNT 2

#define MAC_ADDRESS_LENGTH 20
#define TICKET_URL_LENGTH 128
#define USER_AGENT_LENGTH 32
#define CLIENT_ID_LENGTH 40
#define HOST_NAME_LENGTH 16
#define CLIENT_IP_LENGTH 16
#define SCHOOL_ID_LENGTH 8
#define AUTH_URL_LENGTH 40
#define ALGO_ID_LENGTH 40
#define TICKET_LENGTH 40
#define DOMAIN_LENGTH 16
#define AC_IP_LENGTH 16
#define AREA_LENGTH 8

#define USR_LENGTH 16
#define PWD_LENGTH 128
#define CHN_LENGTH 8

typedef struct
{
    char mac_address[MAC_ADDRESS_LENGTH];
    char ticket_url[TICKET_URL_LENGTH];
    char user_agent[USER_AGENT_LENGTH];
    char client_id[CLIENT_ID_LENGTH];
    char host_name[HOST_NAME_LENGTH];
    char client_ip[CLIENT_IP_LENGTH];
    char school_id[SCHOOL_ID_LENGTH];
    char auth_url[AUTH_URL_LENGTH];
    char algo_id[ALGO_ID_LENGTH];
    char ticket[TICKET_LENGTH];
    char domain[DOMAIN_LENGTH];
    char ac_ip[AC_IP_LENGTH];
    char area[AREA_LENGTH];
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
 * @param index 需要修改的适配器下标
 */
void setOpt(Options opt, int index);

#endif //ESURFINGCLIENT_STATES_H