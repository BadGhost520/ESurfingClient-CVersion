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

/* DialerServer.c */

/**
 * @brief 测两台认证服务器的连接延迟, 选快的那台
 * @param out 输出 "http://ip:7001"
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
