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

/**
 * 登出
 * @return 认证状态码
 */
RunningStatus term();

/**
 * 认证主程序
 */
void dialerApp();

#endif // CLIENT_H