#ifndef ESURFINGCLIENT_STATES_H
#define ESURFINGCLIENT_STATES_H

#include "cipher/CipherInterface.h"

#include "utils/sim/SimThread.h"

#include <inttypes.h>
#include <setjmp.h>
#include <signal.h>

#define SCHOOL_NETWORK_SYMBOL 8

#define TICKET_URL_LEN 512
#define USER_AGENT_LEN 32
#define CLIENT_ID_LEN 40
#define HOST_NAME_LEN 32
#define KEEP_URL_LEN 256
#define TERM_URL_LEN 256
#define AUTH_URL_LEN 256
#define MAC_ADDR_LEN 20
#define TICKET_LEN 40
#define OSTAG_LEN 32

#define USR_LEN 16
#define PWD_LEN 128

#define WEEK_MINUTES 10080
#define MAX_TIME_WINDOWS 16
#define TIME_WINDOW_STR_LEN 32

#define IP_LEN 16
#define IF_LEN 16

#define LOCATION_LEN 512
#define LAST_LOCATION_LEN 1024

#define CONTROL_TOKEN_LEN 33

#define CONTROL_TOKEN_ENV "ESURFING_CONTROL_TOKEN"

#define DEFAULT_CONN_TIMEOUT 7

#define DEFAULT_OP_TIMEOUT 10

#define DEFAULT_CHANNEL 3

#define DEFAULT_WEB_PORT 8888

#define DEFAULT_WEB_EXTERNAL_ACC false

typedef enum
{
    GET_TICKET = 1,
    LOGIN = 2,
    HEART_BEAT = 3,
    TERM = 4
} XmlChoose;

typedef enum
{
    CONSOLE_FORMAT = 1,
    FILE_FORMAT = 2
} TimeFormat;

typedef struct
{
    uint8_t* data;
    size_t len;
} bytes_t;

typedef enum
{
    ROLE_STANDALONE = 0,
    ROLE_SUPERVISOR = 1,
    ROLE_AUTH = 2,
    ROLE_WEB = 3
} prog_role_t;

typedef struct
{
    char ticket_url[TICKET_URL_LEN];
    char client_id[CLIENT_ID_LEN];
    char host_name[HOST_NAME_LEN];
    char ostag[OSTAG_LEN];
    char keep_url[KEEP_URL_LEN];
    char term_url[TERM_URL_LEN];
    char auth_url[AUTH_URL_LEN];
    char mac_addr[MAC_ADDR_LEN];
    char algo_id[ALGO_ID_LEN];
    char ticket[TICKET_LEN];
    char client_ip[IP_LEN];
    char ac_ip[IP_LEN];
    bool dynamic;
    cipher_interface_t* cipher;
    ios_zsm_blob_t blob;
    uint64_t keep_retry;
    uint64_t auth_time;
    uint64_t tick;
    int8_t type;
} auth_cfg_t;

typedef struct
{
    uint16_t start_week_min;
    uint16_t end_week_min;
} time_window_t;

typedef struct
{
    char usr[USR_LEN];
    char pwd[PWD_LEN];
    uint8_t chn;
    char user_agent[USER_AGENT_LEN];
    uint32_t mark;
    bool use_cus_mark;
    time_window_t time_windows[MAX_TIME_WINDOWS];
    uint8_t time_window_count;
    bool has_time_control;
    uint8_t idx;
} login_cfg_t;

typedef struct
{
    bool is_initialized;
    bool is_running;
    bool is_authed;
    bool is_need_reauth;
    bool is_time_disabled;
} runtime_status_t;

typedef struct
{
    auth_cfg_t auth_cfg;
    login_cfg_t login_cfg;
    runtime_status_t runtime_status;
    uint64_t thread_id;
    sim_thread_t* thread;
    char last_location[LAST_LOCATION_LEN * 2];
    bool last_location_lock;
} prog_status_t;

#ifdef _WIN32
extern jmp_buf g_exit_jmp;
#endif

extern uint64_t g_start_run_tm;

extern prog_role_t g_prog_role;

extern uint8_t g_prog_account;

extern uint16_t g_control_port;

extern uint16_t g_web_port;

extern bool g_web_external_acc;

extern char g_control_token[CONTROL_TOKEN_LEN];

extern int g_main_argc;

extern char** g_main_argv;

extern volatile sig_atomic_t g_stop_requested;

extern int8_t g_prog_cnt;

extern _Thread_local int8_t tl_thread_idx;

extern _Thread_local const char* tl_thread_name;

extern prog_status_t* g_prog_status;

extern char g_school_network_symbol[SCHOOL_NETWORK_SYMBOL];

extern bool g_thread_keep_alive;

extern bool g_is_webserver_running;

extern bool g_need_exit;

extern bool g_prog_enabled;

extern bool g_need_restart;

extern bool g_cfg_loaded;

extern long g_conn_timeout;

extern long g_op_timeout;

/** @brief 刷新状态函数 */
void refresh_states();

#endif
