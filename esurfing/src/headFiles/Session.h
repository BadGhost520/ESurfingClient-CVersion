//
// Created by bad_g on 2025/9/24.
//
#ifndef ESURFINGCLIENT_SESSION_H
#define ESURFINGCLIENT_SESSION_H

#include "utils/ByteArray.h"

/**
 * 初始化会话
 * @param zsm zsm 数据流
 */
void initialize(const ByteArray* zsm);

/**
 * 释放会话资源函数
 */
void sessionFree();

/**
 * 加密函数
 * @param text 要加密的文本
 * @return 加密后的数据
 */
char* sessionEncrypt(const char* text);

/**
 * 解密函数
 * @param text 要解密的数据
 * @return 解密后的文本
 */
char* sessionDecrypt(const char* text);

#endif //ESURFINGCLIENT_SESSION_H