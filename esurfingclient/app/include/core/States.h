#ifndef ESURFINGCLIENT_STATES_H
#define ESURFINGCLIENT_STATES_H

#include "cipher/CipherInterface.h"

#include "utils/log/Logger.h"
#include "utils/sim/SimThread.h"

#include <inttypes.h>
#include <setjmp.h>
#include <signal.h>

#define SCHOOL_NETWORK_SYMBOL 8

/* ---- 监管进程的子进程表 (原来在 supervisor/SupervisorState.c) ---- */
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
typedef HANDLE child_handle_t;
#define CHILD_HANDLE_INVALID NULL
typedef volatile LONG stop_flag_t;
#else
#include <signal.h>
#include <sys/types.h>
typedef pid_t child_handle_t;
#define CHILD_HANDLE_INVALID ((pid_t)-1)
typedef volatile sig_atomic_t stop_flag_t;
#endif

#define SUPERVISOR_MAX_CHILDREN 17

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

extern child_t g_children[SUPERVISOR_MAX_CHILDREN];

extern stop_flag_t g_supervisor_stop;

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

#define CFG_LOG_DIR_LEN 512

/** 探测目标: 域名 + 固定 IP */
#define PROBE_HOST_MAX 8
#define PROBE_IP_MAX 4

typedef struct
{
    char host[128];
    char ips[PROBE_IP_MAX][46];
    uint8_t ip_count;
} probe_host_t;

#define WEEK_MINUTES 10080
#define MAX_TIME_WINDOWS 16
#define TIME_WINDOW_STR_LEN 32

#define IP_LEN 16
#define IF_LEN 16

#define LOCATION_LEN 512
#define LAST_LOCATION_LEN 1024

#define CONTROL_TOKEN_LEN 33

#define CONTROL_TOKEN_ENV "ESURFING_CONTROL_TOKEN"

#define DEFAULT_CONN_TIMEOUT 10

#define DEFAULT_OP_TIMEOUT 15

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
    char gwip[IP_LEN];
    cipher_interface_t* cipher;
    uint64_t keep_retry;
    uint64_t auth_time;
    uint64_t tick;
} auth_cfg_t;

typedef struct
{
    uint16_t start_week_min;
    uint16_t end_week_min;
} time_window_t;

typedef struct
{
    /* ---- 账号相关 ---- */
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

    /* ---- 全局配置 (每个账号一份, 值相同; 保存时用第一份) ---- */
    bool enabled;
    bool web_external_acc;
    long log_lv;
    char log_dir[CFG_LOG_DIR_LEN];
    uint16_t web_port;
    long conn_timeout;
    long op_timeout;
    probe_host_t probe_hosts[PROBE_HOST_MAX];
    uint8_t probe_host_count;
} cfg_t;

#define AUTH_ERROR_CODE_LEN 24
#define AUTH_ERROR_MSG_LEN 128

typedef struct
{
    bool is_initialized;
    bool is_running;
    bool is_authed;
    bool is_need_reauth;
    bool is_time_disabled;
    uint8_t last_error_kind;
    uint64_t last_error_at;
    char last_error_code[AUTH_ERROR_CODE_LEN];
    char last_sub_error[AUTH_ERROR_CODE_LEN];
    char last_error_msg[AUTH_ERROR_MSG_LEN];
} runtime_status_t;

typedef struct
{
    auth_cfg_t auth_cfg;
    cfg_t cfg;
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

/** 收尾是否已经跑完 (关控制台窗口时处理线程靠它决定要不要再等) */
extern volatile sig_atomic_t g_shutdown_done;

/**
 * 是不是"关窗口/注销/关机"这类系统只给几秒的事件
 *
 * 与 Ctrl+C 不同: 这类事件处理完就得马上走, 否则进程会被系统强杀 ——
 * 监管进程据此把等待子进程的时间压短, 保证还来得及归档日志。
 */
extern volatile sig_atomic_t g_console_closed;

/** 启动标识长度: 8 字节随机数的十六进制 + 结尾 NUL */
#define BOOT_ID_LEN 17

/**
 * @brief 取本次进程启动的唯一标识 (第一次调用时随机生成)
 *
 * 网页靠它判断"重启完了没有": 重启前记下旧值, 变了就说明新进程已经起来了。
 * 仅用于展示与判定, 不参与安全校验。
 * @return 16 个十六进制字符 (进程内静态, 不要 free)
 */
const char* boot_id_get(void);

extern int8_t g_prog_cnt;

extern _Thread_local int8_t tl_thread_idx;

extern _Thread_local const char* tl_thread_name;

extern prog_status_t* g_prog_status;

extern char g_school_network_symbol[SCHOOL_NETWORK_SYMBOL];

/* 门户页面响应头里的 School Id / Domain / Area (都是短串) */
#define SCHOOL_ID_LENGTH 8
#define DOMAIN_LENGTH 16
#define AREA_LENGTH 8

extern char g_school_id[SCHOOL_ID_LENGTH];

extern char g_school_domain[DOMAIN_LENGTH];

extern char g_area[AREA_LENGTH];

/* 日志模块的全局状态 (原来是 LogCore.c 里的定义) */
extern const char g_log_file_name[];

extern char g_cfg_log_dir[LOG_PATH_MAX];

extern bool g_logger_inited;

extern log_cfg_t g_logger_cfg;

extern bool g_console_enabled;

extern bool g_query_mode;

/* --list-accounts / --print-log-dir 这类查询模式 (不走完整启动流程) */
extern bool g_list_only;

/* 监管进程拉起的子进程数 (子进程表本身在 SupervisorState.c) */
extern int g_child_count;

extern bool g_thread_keep_alive;

extern bool g_is_webserver_running;

extern bool g_need_exit;

extern bool g_prog_enabled;

extern bool g_need_restart;

extern bool g_cfg_loaded;

extern bool g_need_reload;

extern bool g_waiting_cfg;

extern long g_conn_timeout;

extern long g_op_timeout;

/** @brief 刷新状态函数 */
void refresh_states(void);

#endif
