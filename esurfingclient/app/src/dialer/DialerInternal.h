#ifndef ESURFINGCLIENT_DIALERINTERNAL_H
#define ESURFINGCLIENT_DIALERINTERNAL_H

#include "core/States.h"

#include <stdbool.h>

typedef enum
{
    AUTH_SUCCESS = 0,
    AUTH_FAILED = 1,
    INIT_SESSION_FAILED = 2,
    GET_TICKET_FAILED = 3,
    LOGIN_FAILED = 4
} AuthStatus;

typedef enum
{
    WAIT_READY = 0,
    WAIT_RETRY_EXHAUSTED = 1,
    WAIT_EXIT = 2,
    WAIT_TIME_CLOSED = 3,
    WAIT_CFG_CHANGED = 4
} WaitResult;

/*
 * 本模块没有文件级可变状态: 会话、配置与运行状态都在 States.h 的 g_prog_status
 * (以及 g_* 全局) 里, 拆分前后是同一个对象。唯一的文件级数据是 run() 用的
 * 只读常量表, 留在 DialerClient.c。
 */

/** 单次请求的失败类型: 让调用方区分"网络不通"和"服务端拒绝" */
typedef enum
{
    DIALER_FAIL_NONE = 0,
    DIALER_FAIL_NET,    // curl 层失败 (连不上/超时)
    DIALER_FAIL_SERVER  // 服务端有响应, 但状态码或内容不对
} dialer_fail_t;

/** 心跳结果: 决定失败后是"退避重试"还是"重新认证" */
typedef enum
{
    HB_OK = 0,
    HB_NET_FAIL,   // 网络层失败 —— 重认证没有意义, 等网络恢复
    HB_PROTO_FAIL  // 服务端有响应但内容不对 —— 可能已被踢下线, 需要重认证
} hb_result_t;

/* DialerNet.c */
bool term(void);

hb_result_t heartbeat(void);

bool login(void);

/** 构造/加密 → POST → 解密 (心跳 / 登录 / 取 ticket 共用) */
char* dialer_request_plain(XmlChoose kind, const char* url, const char* what, dialer_fail_t* fail);

/** 获取 Ticket，处理 Error-Code=200 + ZSM 的官方 algoid 更新流程 */
char* dialer_request_ticket(XmlChoose kind, const char* url, const char* what);

WaitResult wait_need_auth(void);

void logout_previous_session(void);

/* DialerTicket.c */
bool get_ticket(void);

bool load_cipher(bytes_t zsm);

/* DialerAuth.c */
AuthStatus auth(void);

int work_auth(void);

/* DialerLocation.c */
bool get_last_location(void);

/**
 * @brief 主动重新探测一次网关并跟随跳转刷新 last_location (重认证防陈旧)
 */
bool refresh_last_location(void);

/* DialerServer.c */
#define AUTH_CANDIDATE_MAX 16

/**
 * @brief 将输入归一化为 "host:port" (默认端口 7001, 拒绝 https/非法字符)
 */
bool auth_server_normalize(const char* raw, char* out, size_t out_len);

/**
 * @brief 把运行期发现的集群节点加入动态候选池 (自动去重)
 */
void auth_server_add_dynamic(const char* raw);

/**
 * @brief 从门户 EConfig 与重定向 URL 提取集群/认证节点加入候选池
 */
void auth_server_extract_econfig(const char* portal_config, const char* last_location);

/**
 * @brief 标记某台服务器本次失败 (进入冷却窗, 优先尝试其它健康节点)
 */
void auth_server_mark_failed(const char* server_url_or_hostport);

/**
 * @brief 标记某台服务器成功, 清除其失败冷却状态
 */
void auth_server_mark_ok(const char* server_url_or_hostport);

/**
 * @brief 清除所有认证服务器的失败冷却记录
 */
void auth_server_clear_failures(void);

/**
 * @brief 并发测速并按健康度与延迟排序候选认证服务器 ("http://host:port")
 */
size_t select_auth_servers(char out[][AUTH_SERVER_LEN + 16], size_t max_count);

/**
 * @brief 测可用认证服务器的连接延迟, 选最健康且最快的那台
 * @param out 输出 "http://host:port"
 * @param out_len 缓冲长度
 * @return 是否至少测通一台
 */
bool select_auth_server(char* out, size_t out_len);

/* DialerSession.c */
bool init_session(void);

void clean(void);

void reset(void);

bool supervisor_gone(void);

/* DialerClient.c */
int dialer_app(void* arg);

void print_banner(void);

#endif
