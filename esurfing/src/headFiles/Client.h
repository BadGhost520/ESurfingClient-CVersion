#ifndef CLIENT_H
#define CLIENT_H

typedef struct
{
    long long tick;
    char* keep_retry;
    char* keep_url;
    char* term_url;
} ClientData;

/**
 * 主运行函数
 */
void run();

/**
 * 登出函数
 */
void term();

#endif // CLIENT_H