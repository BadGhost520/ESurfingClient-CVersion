//
// PlatformUtils.h - 跨平台工具函数
// Created by bad_g on 2025/9/14.
//

#ifndef PLATFORMUTILS_H
#define PLATFORMUTILS_H

// 跨平台睡眠函数
// 参数单位：秒
void sleep_seconds(int seconds);

// 跨平台睡眠函数
// 参数单位：毫秒
void sleep_milliseconds(int milliseconds);

// 跨平台睡眠函数
// 参数单位：微秒
void sleep_microseconds(int microseconds);

#endif // PLATFORMUTILS_H