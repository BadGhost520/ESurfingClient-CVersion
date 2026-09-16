#ifndef ESURFINGCLIENT_STATES_H
#define ESURFINGCLIENT_STATES_H

#include "cipher/CipherInterface.h"
#include "utils/sim/SimThread.h"

#include <setjmp.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>

#define SCHOOL_NETWORK_SYMBOL 8

#define TICKET_URL_LEN 512
#define USER_AGENT_LEN 32
#define CLIENT_ID_LEN 40
#define HOST_NAME_LEN 32
#define OSTAG_LEN 32
#define KEEP_URL_LEN 256
#define TERM_URL_LEN 256
#define AUTH_URL_LEN 256
#define MAC_ADDR_LEN 20
#define ALGO_ID_LEN 37
#define TICKET_LEN 40

#define USR_LEN 16
#define PWD_LEN 128

#define WEEK_MINUTES 10080
#define MAX_TIME_WINDOWS 16
#define TIME_WINDOW_STR_LEN 32

#define IP_LEN 16
#define IF_LEN 16

#define LOCATION_LEN 512
#define LAST_LOCATION_LEN 1024

#define WEB_LISTEN_LEN 64

/**
 * @brief Web 服务默认监听地址
 *
 * 默认只监听回环: /api/getConfigs 会返回明文账号密码, 而服务本身没有鉴权,
 * 监听 0.0.0.0 等于把这些暴露给整个局域网
 */
#define DEFAULT_WEB_LISTEN "127.0.0.1:8888"

/** @brief 程序角色 */
typedef enum
{
    /** @brief 单进程模式 (未指定 --role, 保持原有行为) */
    ROLE_STANDALONE = 0,
    /** @brief 守护进程 */
    ROLE_SUPERVISOR = 1,
    /** @brief 认证进程 */
    ROLE_AUTH = 2,
    /** @brief Web 服务进程 */
    ROLE_WEB = 3
} prog_role_t;

/** @brief 认证配置 */
typedef struct
{
    /** @brief 票据 URL */
    char ticket_url[TICKET_URL_LEN];
    /** @brief 客户端 ID */
    char client_id[CLIENT_ID_LEN];
    /** @brief 主机名 */
    char host_name[HOST_NAME_LEN];
    /** @brief 系统标识 */
    char ostag[OSTAG_LEN];
    /** @brief 心跳 URL */
    char keep_url[KEEP_URL_LEN];
    /** @brief 登出 URL */
    char term_url[TERM_URL_LEN];
    /** @brief 认证 URL */
    char auth_url[AUTH_URL_LEN];
    /** @brief MAC 地址 */
    char mac_addr[MAC_ADDR_LEN];
    /** @brief 加解密 ID */
    char algo_id[ALGO_ID_LEN];
    /** @brief 票据 */
    char ticket[TICKET_LEN];
    /** @brief 客户端 IP */
    char client_ip[IP_LEN];
    /** @brief 服务端 IP */
    char ac_ip[IP_LEN];
    /** @brief 加解密工厂 */
    cipher_interface_t* cipher;
    /** @brief 重试时间 */
    uint64_t keep_retry;
    /** @brief 认证时间 */
    uint64_t auth_time;
    /** @brief 当前时间 (用于检测认证时间) */
    uint64_t tick;
} auth_cfg_t;

/** @brief 一周时间窗口 */
typedef struct
{
    /** @brief 开始周分钟 (0-10079, 0=周日 00:00) */
    uint16_t start_week_min;
    /** @brief 结束周分钟 (可大于 10080, 用于跨周窗口) */
    uint16_t end_week_min;
} time_window_t;

/** @brief 登录配置 */
typedef struct
{
    /** @brief 用户名 */
    char usr[USR_LEN];
    /** @brief 密码 */
    char pwd[PWD_LEN];
    /** @brief 认证通道 */
    uint8_t chn;
    /** @brief 设备 UA */
    char user_agent[USER_AGENT_LEN];
    /** @brief 标记值 */
    uint32_t mark;
    /** @brief 是否使用自定义标记值 */
    bool use_cus_mark;
    /** @brief 一周时间窗口列表 */
    time_window_t time_windows[MAX_TIME_WINDOWS];
    /** @brief 有效时间窗口数量 */
    uint8_t time_window_count;
    /** @brief 是否启用时间控制 */
    bool has_time_control;
    /** @brief 配置序号 */
    uint8_t idx;
} login_cfg_t;

/** @brief 运行状态 */
typedef struct
{
    /** @brief 初始化状态 */
    bool is_initialized;
    /** @brief 运行状态 */
    bool is_running;
    /** @brief 认证状态 */
    bool is_authed;
    /** @brief 需要重新认证 */
    bool is_need_reauth;
    /** @brief 时间控制禁用中 (仅内存状态, 不落盘) */
    bool is_time_disabled;
} runtime_status_t;

/** @brief 认证线程状态 */
typedef struct
{
    /** @brief 认证配置 */
    auth_cfg_t auth_cfg;
    /** @brief 登录配置 */
    login_cfg_t login_cfg;
    /** @brief 运行状态 */
    runtime_status_t runtime_status;
    /** @brief 线程 ID */
    uint64_t thread_id;
    /** @brief 线程 */
    sim_thread_t* thread;
    /** @brief 获取认证配置地址 */
    char last_location[LAST_LOCATION_LEN * 2];
    /** @brief last_location 数据锁 */
    bool last_location_lock;
} prog_status_t;

#ifdef _WIN32
/** @brief 跳出标志 (用于 Windows 服务退出) */
extern jmp_buf g_exit_jmp;
#endif

/** @brief 程序开始运行时间 */
extern uint64_t g_start_run_tm;

/** @brief 当前程序角色 */
extern prog_role_t g_prog_role;

/** @brief 当前进程负责的配置序号 (配置文件中的原始下标, 从 1 开始, 0 表示未指定) */
extern uint8_t g_prog_account;

/** @brief 控制通道端口 */
extern uint16_t g_control_port;

/** @brief Web 服务监听地址 (形如 127.0.0.1:8888) */
extern char g_web_listen[WEB_LISTEN_LEN];

/** @brief 主函数收到的参数个数 (重启时要原样带上) */
extern int g_main_argc;

/** @brief 主函数收到的参数 (重启时要原样带上) */
extern char** g_main_argv;

/**
 * @brief 收到退出请求
 *
 * 只由信号处理函数置位: 那里不能做 join / 打日志 / rename / exit 这些
 * 不是 async-signal-safe 的事, 真正的关闭动作由各角色的主循环来做
 */
extern volatile sig_atomic_t g_stop_requested;

/** @brief 适配器数 */
extern int8_t g_prog_cnt;

/** @brief 线程独立下标 */
extern _Thread_local int8_t tl_thread_idx;

/** @brief 认证线程状态 */
extern prog_status_t* g_prog_status;

/** @brief 校园网标志 */
extern char g_school_network_symbol[SCHOOL_NETWORK_SYMBOL];

/** @brief 线程保活 */
extern bool g_thread_keep_alive;

/** @brief Web 服务器运行状态 */
extern bool g_is_webserver_running;

/** @brief 需要退出 */
extern bool g_need_exit;

/** @brief 程序启用状态 */
extern bool g_prog_enabled;

/** @brief 需要重启 */
extern bool g_need_restart;

/** @brief 配置文件加载状态 */
extern bool g_cfg_loaded;

/** @brief 连接超时时长 */
extern long g_conn_timeout;

/** @brief 总操作超时时长 */
extern long g_op_timeout;

/** @brief 刷新状态函数 */
void refresh_states();

#endif //ESURFINGCLIENT_STATES_H
