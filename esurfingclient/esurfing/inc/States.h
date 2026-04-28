#ifndef ESURFINGCLIENT_STATES_H
#define ESURFINGCLIENT_STATES_H

#include "cipher/CipherInterface.h"
#include "utils/SimThread.h"

#include <stdint.h>

#define SCHOOL_NETWORK_SYMBOL 8

#define TICKET_URL_LEN 256
#define USER_AGENT_LEN 32
#define CLIENT_ID_LEN 40
#define HOST_NAME_LEN 16
#define KEEP_URL_LEN 80
#define TERM_URL_LEN 80
#define AUTH_URL_LEN 80
#define MAC_ADDR_LEN 20
#define ALGO_ID_LEN 37
#define TICKET_LEN 40

#define USR_LEN 16
#define PWD_LEN 128
#define CHN_LEN 8

#define IP_LEN 16
#define IF_LEN 16

#define LAST_LOCATION_LEN 256

/** @brief 认证配置 */
typedef struct
{
    /** @brief 票据 URL */
    char ticket_url[TICKET_URL_LEN];
    /** @brief 客户端 ID */
    char client_id[CLIENT_ID_LEN];
    /** @brief 主机名 */
    char host_name[HOST_NAME_LEN];
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
    cipherInterfaceT* cipher;
    /** @brief 重试时间 */
    uint64_t keep_retry;
    /** @brief 认证时间 */
    uint64_t auth_time;
    /** @brief 当前时间 (用于检测认证时间) */
    uint64_t tick;
} auth_cfg_t;

/** @brief 登录配置 */
typedef struct
{
    /** @brief 用户名 */
    char usr[USR_LEN];
    /** @brief 密码 */
    char pwd[PWD_LEN];
    /** @brief 认证通道 */
    char chn[CHN_LEN];
    /** @brief 设备 UA */
    char user_agent[USER_AGENT_LEN];
    /** @brief 认证使用的网卡 */
    char i_f[IF_LEN];
    /** @brief 自启状态 */
    bool auto_start;
    /** @brief 配置序号 */
    uint8_t idx;
} login_cfg_t;

/** @brief 运行状态 */
typedef struct
{
    // /** @brief 设置是否变化 */
    // bool is_settings_changed;
    /** @brief 初始化状态 */
    bool is_initialized;
    /** @brief 运行状态 */
    bool is_running;
    /** @brief 认证状态 */
    bool is_authed;
    /** @brief 需要重置 */
    bool is_need_reset;
    /** @brief last_location 数据锁 */
    bool last_location_lock;
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
    /** @brief 获取认证配置地址 */
    char last_location[LAST_LOCATION_LEN];
    /** @brief 标记值 */
    uint32_t mark;
    /** @brief 线程 ID */
    uint64_t thread_id;
    /** @brief 线程 */
    sim_thread_t* thread;
} prog_status_t;

/** @brief 程序开始运行时间 */
// extern uint64_t g_start_run_tm;

/** @brief 适配器数 */
extern int8_t g_prog_cnt;

/** @brief 线程独立下标 */
extern _Thread_local int8_t thread_idx;

/** @brief 网络检测锁 */
extern bool check_net_lock;

#ifdef __OPENWRT__
/** @brief 是否使用自定义 IP */
extern bool g_use_cus_if;
#endif

/** @brief 线程保活 */
extern bool thread_keep_alive;

/** @brief 认证线程状态 */
extern prog_status_t* g_prog_status;

/** @brief 校园网标志 */
extern char school_network_symbol[SCHOOL_NETWORK_SYMBOL];

/** @brief 刷新状态函数 */
void refresh_states();

#endif //ESURFINGCLIENT_STATES_H
