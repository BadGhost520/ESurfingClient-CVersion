#ifndef ESURFINGCLIENT_STATES_H
#define ESURFINGCLIENT_STATES_H

#include <stdbool.h>
#include <stdint.h>

#define MAC_ADDRESS_LENGTH 20
#define TICKET_URL_LENGTH 256
#define USER_AGENT_LENGTH 32
#define KEEP_RETRY_LENGTH 8
#define CLIENT_ID_LENGTH 40
#define HOST_NAME_LENGTH 16
#define KEEP_URL_LENGTH 80
#define TERM_URL_LENGTH 80
#define AUTH_URL_LENGTH 80
#define ALGO_ID_LENGTH 37
#define TICKET_LENGTH 40

#define USR_LENGTH 16
#define PWD_LENGTH 128
#define CHN_LENGTH 8

#define IP_LENGTH 16
#define ADAPTER_NAME_LENGTH 128

/** @brief 认证配置 */
typedef struct
{
    /** @brief MAC 地址 */
    char mac_address[MAC_ADDRESS_LENGTH];
    /** @brief 票据 URL */
    char ticket_url[TICKET_URL_LENGTH];
    /** @brief 设备 UA */
    char user_agent[USER_AGENT_LENGTH];
    /** @brief 重试时间 */
    char keep_retry[KEEP_RETRY_LENGTH];
    /** @brief 客户端 ID */
    char client_id[CLIENT_ID_LENGTH];
    /** @brief 主机名 */
    char host_name[HOST_NAME_LENGTH];
    /** @brief 认证 URL */
    char auth_url[AUTH_URL_LENGTH];
    /** @brief 心跳 URL */
    char keep_url[KEEP_URL_LENGTH];
    /** @brief 登出 URL */
    char term_url[TERM_URL_LENGTH];
    /** @brief 加解密 ID */
    char algo_id[ALGO_ID_LENGTH];
    /** @brief 票据 */
    char ticket[TICKET_LENGTH];
    /** @brief 客户端 IP */
    char client_ip[IP_LENGTH];
    /** @brief 服务端 IP */
    char ac_ip[IP_LENGTH];
    /** @brief 当前时间 */
    long long tick;
} AuthConfig;

/** @brief 登录配置 */
typedef struct
{
    /** @brief 用户名 */
    char usr[USR_LENGTH];
    /** @brief 密码 */
    char pwd[PWD_LENGTH];
    /** @brief 认证通道 */
    char chn[CHN_LENGTH];
    /** @brief 认证使用的 IP */
    char ip[IP_LENGTH];
    /** @brief 自启状态 */
    bool auto_start;
} LoginConfig;

/** @brief 运行状态 */
typedef struct
{
    /** @brief 设置是否变化 */
    bool is_settings_changed;
    /** @brief 初始化状态 */
    bool is_initialized;
    /** @brief 运行状态 */
    bool is_running;
    /** @brief 认证状态 */
    bool is_authed;
    /** @brief 认证时间 */
    int64_t auth_time;
} RuntimeStatus;

/** @brief 主认证程序状态 */
typedef struct
{
    /** @brief 认证配置 */
    AuthConfig auth_config;
    /** @brief 登录配置 */
    LoginConfig login_config;
    /** @brief 运行状态 */
    RuntimeStatus runtime_status;
} ProgStatus;

/** @brief 全局运行时间 */
extern int64_t g_running_time;

/** @brief 主程序状态 */
extern ProgStatus* prog_status;

/** @brief 适配器数 */
extern int prog_count;

/** @brief 正在操作的适配器下标 */
extern int prog_index;

/** @brief 刷新状态函数 */
void refreshStates();

#endif //ESURFINGCLIENT_STATES_H