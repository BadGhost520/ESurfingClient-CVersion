//
// PlatformUtils.h - 跨平台工具函数
// Created by bad_g on 2025/9/14.
//

#ifndef PLATFORMUTILS_H
#define PLATFORMUTILS_H

#include <stdint.h>

int stringToLongLong(const char* str, long long* result);

int64_t currentTimeMillis();

void sleepMilliseconds(int milliseconds);

void setClientId(char** client_id);

char* randomMacAddress();

char* randomString();

char* getTime();

char* createXMLPayload(const char* choose);

char* cleanCDATA(const char* text);

#endif // PLATFORMUTILS_H