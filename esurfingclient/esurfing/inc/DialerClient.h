#ifndef CLIENT_H
#define CLIENT_H

typedef enum
{
    RUNNING_FAILURE = 0,
    RUNNING_SUCCESS = 1
} RunningStatus;

typedef enum
{
    AUTH_FAILURE = 0,
    AUTH_SUCCESS = 1
} AuthStatus;

typedef enum
{
    INIT_FAILURE = 0,
    INIT_SUCCESS = 1
} InitStatus;

/**
 * @brief 认证线程
 * @param arg 传入参数
 * @return 线程返回值
 */
int dialer_app(void* arg);

#endif // CLIENT_H
