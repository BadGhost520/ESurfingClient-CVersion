//
// Created by bad_g on 2025/9/24.
//
#ifndef ESURFINGCLIENT_SESSION_H
#define ESURFINGCLIENT_SESSION_H

#include "utils/ByteArray.h"

// Session初始化和状态检查
void initialize(const ByteArray* zsm);

int isInitialized();

// 十六进制字符串转换函数
int hex_string_to_binary(const char* hex_str, size_t hex_len, unsigned char** binary_data, size_t* binary_len);

// 资源管理
void sessionFree();

int init_cipher(const char* algo_id);

// 加解密
char* sessionEncrypt(const char* text);
char* sessionDecrypt(const char* text);

#endif //ESURFINGCLIENT_SESSION_H