#ifndef CLIENT_H
#define CLIENT_H

typedef struct
{
    long long tick;
    char* keep_retry;
    char* keep_url;
    char* term_url;
} ClientData;

typedef enum
{
    RUNNING_FAILURE = 0,
    RUNNING_SUCCESS = 1,
} RunningStatus;

typedef enum
{
    AUTH_FAILURE = 0,
    AUTH_SUCCESS = 1,
} AuthStatus;

/**
 * 主运行函数
 * @return 运行状态码
 */
RunningStatus run();

/**
 * 登出函数
 * @return 认证状态码
 */
RunningStatus term();

#endif // CLIENT_H