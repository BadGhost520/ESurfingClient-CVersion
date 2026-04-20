#ifndef CLIENT_H
#define CLIENT_H

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

typedef enum
{
    INIT_FAILURE = 0,
    INIT_SUCCESS = 1
} InitStatus;

/**
 * @brief 登出
 * @return 认证状态码
 */
RunningStatus term();

/**
 * @brief 认证主程序
 */
void dialer_app();

#endif // CLIENT_H
