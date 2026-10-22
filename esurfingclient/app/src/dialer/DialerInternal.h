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

/* DialerNet.c */
bool term(void);

bool heartbeat(void);

bool login(void);

/** 构造/加密 → POST → 解密 (心跳 / 登录 / 取 ticket 共用) */
char* dialer_request_plain(XmlChoose kind, const char* url, const char* what);

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

/* DialerSession.c */
bool init_session(void);

void clean(void);

void reset(void);

bool supervisor_gone(void);

/* DialerClient.c */
int dialer_app(void* arg);

void print_banner(void);

#endif
