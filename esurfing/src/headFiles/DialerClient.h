#ifndef CLIENT_H
#define CLIENT_H

#include "States.h"

#define KEEP_RETRY_LENGTH 8
#define KEEP_URL_LENGTH 80
#define TERM_URL_LENGTH 80

typedef struct
{
    long long tick;
    char keep_retry[KEEP_RETRY_LENGTH];
    char keep_url[KEEP_URL_LENGTH];
    char term_url[TERM_URL_LENGTH];
} ClientData;

typedef struct
{
    int thread_index;
    char ip[IP_LENGTH];
    bool can_run;
} ThreadArgs;

typedef enum
{
    RUNNING_FAILURE = 0,
    RUNNING_SUCCESS = 1,
    RUNNING_WARNING = 2,
} RunningStatus;

typedef enum
{
    AUTH_FAILURE = 0,
    AUTH_SUCCESS = 1,
} AuthStatus;

extern __thread ClientData client_data;
extern __thread ThreadArgs thread_local_args;
extern ThreadArgs args[MAX_DIALER_COUNT];

/**
 * 登出函数
 * @return 认证状态码
 */
RunningStatus term();

/**
 * 认证线程主函数
 * @param arg 参数
 * @return NULL
 */
void* dialerApp(void* arg);

#endif // CLIENT_H