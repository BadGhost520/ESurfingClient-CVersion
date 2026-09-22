#ifndef ESURFINGCLIENT_DIALER_INTERNAL_H
#define ESURFINGCLIENT_DIALER_INTERNAL_H

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
 * 只读常量表, 留在 dialer_client.c。
 */

/* dialer_net.c */
bool term();

bool heartbeat();

bool login();

WaitResult wait_need_auth();

void logout_previous_session();

/* dialer_ticket.c */
bool get_ticket();

bool load_cipher(const bytes_t zsm);

/* dialer_auth.c */
AuthStatus auth();

int work_auth();

/* dialer_location.c */
bool get_last_location();

/* dialer_session.c */
bool init_session();

void clean();

void reset();

bool supervisor_gone();

/* dialer_client.c */
int dialer_app(void* arg);

void print_banner();

#endif
