#ifndef ESURFINGCLIENT_SESSION_H
#define ESURFINGCLIENT_SESSION_H

#include "utils/PlatformUtils.h"

typedef enum
{
    INIT_FAILURE = 0,
    INIT_SUCCESS = 1
} InitStatus;

/**
 * 初始化会话
 * @param zsm zsm 数据流
 */
void initialize(ByteArray zsm);

/**
 * 释放会话资源函数
 */
void freeSession();

#endif //ESURFINGCLIENT_SESSION_H