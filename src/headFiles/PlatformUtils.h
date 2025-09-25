//
// PlatformUtils.h - 跨平台工具函数
// Created by bad_g on 2025/9/14.
//

#ifndef PLATFORMUTILS_H
#define PLATFORMUTILS_H

// 跨平台睡眠函数
// 参数单位：秒
void sleepSeconds(int seconds);

// 跨平台睡眠函数
// 参数单位：毫秒
void sleepMilliseconds(int milliseconds);

// 跨平台睡眠函数
// 参数单位：微秒
void sleepMicroseconds(int microseconds);

void setClientId(char** client_id);

char* randomMacAddress();

// 创建XML payload字符串
char* createXMLPayload();

#endif // PLATFORMUTILS_H