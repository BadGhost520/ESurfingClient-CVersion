//
// PlatformUtils.c - 跨平台工具函数实现
// Created by bad_g on 2025/9/14.
//

#include "headFiles/PlatformUtils.h"

// 平台特定的头文件
#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
    #include <time.h>
#endif

// 跨平台睡眠函数 - 秒
void sleep_seconds(int seconds) {
    if (seconds <= 0) return;
    
#ifdef _WIN32
    Sleep(seconds * 1000);  // Windows: 毫秒
#else
    sleep(seconds);         // Unix/Linux: 秒
#endif
}

// 跨平台睡眠函数 - 毫秒
void sleep_milliseconds(int milliseconds) {
    if (milliseconds <= 0) return;
    
#ifdef _WIN32
    Sleep(milliseconds);    // Windows: 毫秒
#else
    usleep(milliseconds * 1000);  // Unix/Linux: 微秒
#endif
}

// 跨平台睡眠函数 - 微秒
void sleep_microseconds(int microseconds) {
    if (microseconds <= 0) return;
    
#ifdef _WIN32
    // Windows没有直接的微秒睡眠，转换为毫秒（精度损失）
    int milliseconds = (microseconds + 999) / 1000;  // 向上取整
    if (milliseconds > 0) {
        Sleep(milliseconds);
    }
#else
    usleep(microseconds);   // Unix/Linux: 微秒
#endif
}