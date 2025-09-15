//
// PlatformUtils.c - 跨平台工具函数实现
// Created by bad_g on 2025/9/14.
//

#include "headFiles/PlatformUtils.h"

#include <stdio.h>
#include <time.h>

// 平台特定的头文件
#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
    #include <time.h>
#endif

// 跨平台睡眠函数 - 秒
void sleepSeconds(int seconds)
{
    if (seconds <= 0) return;
    
#ifdef _WIN32
    Sleep(seconds * 1000);  // Windows: 毫秒
#else
    sleep(seconds);         // Unix/Linux: 秒
#endif
}

// 跨平台睡眠函数 - 毫秒
void sleepMilliseconds(int milliseconds)
{
    if (milliseconds <= 0) return;
    
#ifdef _WIN32
    Sleep(milliseconds);    // Windows: 毫秒
#else
    usleep(milliseconds * 1000);  // Unix/Linux: 微秒
#endif
}

// 跨平台睡眠函数 - 微秒
void sleepMicroseconds(int microseconds)
{
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

void setClientId(char** client_id)
{
    // 分配内存
    *client_id = malloc(37); // 36字符 + '\0'

    if (*client_id) {
        // 初始化随机数种子
        static int seeded = 0;
        if (!seeded) {
            srand((unsigned int)time(NULL));
            seeded = 1;
        }

        // 生成伪UUID格式字符串
        snprintf(*client_id, 37,
            "%08x-%04x-%04x-%04x-%012llx",
            rand(),                           // 8位十六进制
            rand() & 0xFFFF,                 // 4位十六进制
            (rand() & 0x0FFF) | 0x4000,      // 4位十六进制，版本4
            (rand() & 0x3FFF) | 0x8000,      // 4位十六进制，变体位
            ((long long)rand() << 32) | rand() // 12位十六进制
        );

        // 转换为小写
        for (int i = 0; (*client_id)[i]; i++) {
            (*client_id)[i] = tolower((*client_id)[i]);
        }
    }
}

char* randomMacAddress()
{
    // 分配内存存储MAC地址字符串 (格式: xx:xx:xx:xx:xx:xx)
    char* mac_str = (char*)malloc(18 * sizeof(char));
    if (mac_str == NULL) {
        return NULL;
    }

    // 初始化随机数种子
    srand((unsigned int)time(NULL));

    // 生成6个随机字节
    unsigned char mac_bytes[6];
    for (int i = 0; i < 6; i++) {
        mac_bytes[i] = (unsigned char)(rand() % 256);
    }

    // 修正第一字节，确保最低位为0（单播地址）
    mac_bytes[0] = mac_bytes[0] & 0xFE;  // 0xFE = 254 = 11111110

    // 格式化为字符串
    sprintf(mac_str, "%02x:%02x:%02x:%02x:%02x:%02x",
            mac_bytes[0], mac_bytes[1], mac_bytes[2],
            mac_bytes[3], mac_bytes[4], mac_bytes[5]);

    return mac_str;
}