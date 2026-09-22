#ifndef ESURFINGCLIENT_DIALERINTERNAL_H
#define ESURFINGCLIENT_DIALERINTERNAL_H

#include "utils/PlatformUtils.h"

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
    WAIT_TIME_CLOSED = 3
} WaitResult;

/*
 * 本模块没有文件级可变状态: 会话、配置与运行状态都在 States.h 的 g_prog_status
 * (以及 g_* 全局) 里, 拆分前后是同一个对象。唯一的文件级数据是 run() 用的
 * 只读常量表, 留在 DialerClient.c。
 */

/* DialerNet.c */
bool term();

bool heartbeat();

bool login();

WaitResult wait_need_auth();

void logout_previous_session();

/* DialerTicket.c */
bool get_ticket();

bool load_cipher(const bytes_t zsm);

/* DialerAuth.c */
AuthStatus auth();

int work_auth();

/* DialerLocation.c */
bool get_last_location();

/* DialerSession.c */
bool init_session();

void clean();

void reset();

bool supervisor_gone();

/* DialerClient.c */
int dialer_app(void* arg);

void print_banner();

#endif
