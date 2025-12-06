//
// Created by bad_g on 2025/9/24.
//
#ifndef ESURFINGCLIENT_SESSION_H
#define ESURFINGCLIENT_SESSION_H

#include "utils/PlatformUtils.h"

/**
 * 初始化会话
 * @param zsm zsm 数据流
 */
void initialize(const ByteArray* zsm);

/**
 * 释放会话资源函数
 */
void sessionFree();

#endif //ESURFINGCLIENT_SESSION_H