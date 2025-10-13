//
// PlatformUtils.h - 跨平台工具函数
// Created by bad_g on 2025/9/14.
//

#ifndef PLATFORMUTILS_H
#define PLATFORMUTILS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 将字符串转换为64位长整型
 * 等价于 Kotlin 的 String.toLong()（64位版本）
 *
 * @param str 要转换的字符串
 * @param result 转换结果的指针
 * @return 转换是否成功 (1=成功, 0=失败)
 *
 * 示例:
 *   long long value;
 *   if (string_to_long_long("1234567890", &value)) {
 *       printf("转换成功: %lld\n", value);
 *   }
 */
int stringToLongLong(const char* str, long long* result);

/**
 * 便利宏定义
 * 提供更简洁的调用方式
 */

// 等价于 Kotlin: keepRetry.toLong()
#define TO_LONG_LONG(str, result) string_to_long_long(str, result)

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif

    /**
     * 时间工具函数
     * 提供与Java时间相关函数等价的C语言实现
     */

    /**
     * 获取当前时间的毫秒时间戳
     * 等价于 Java 的 System.currentTimeMillis()
     *
     * 返回自1970年1月1日00:00:00 UTC以来的毫秒数
     * 这与Java的System.currentTimeMillis()返回值完全一致
     *
     * @return 毫秒时间戳，失败返回-1
     */
    int64_t currentTimeMillis();

    /**
     * 获取高精度时间戳（纳秒）
     * 等价于 Java 的 System.nanoTime()
     *
     * @return 纳秒时间戳，失败返回-1
     */
    int64_t currentTimeNanos();

    /**
     * 获取单调时间戳（毫秒）
     * 不受系统时间调整影响，适用于计算时间间隔
     * 类似于Java的System.nanoTime()但返回毫秒
     *
     * @return 单调时间戳（毫秒），失败返回-1
     */
    int64_t monotonicTimeMillis();

    /**
     * 格式化时间戳为可读字符串
     * 格式：YYYY-MM-DD HH:MM:SS.mmm
     *
     * @param timestamp_ms 毫秒时间戳
     * @param buffer 输出缓冲区（至少32字节）
     * @param buffer_size 缓冲区大小
     * @return 格式化后的字符串长度，失败返回-1
     */
    int formatTimestamp(int64_t timestamp_ms, char* buffer, size_t buffer_size);

    /**
     * 便利宏定义
     */
#define GET_CURRENT_TIME_MILLIS() current_time_millis()
#define GET_TICK_COUNT() current_time_millis()

#ifdef __cplusplus
}
#endif

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
char* createXMLPayload(const char* choose);

// 清除 CDATA 数据
char* cleanCDATA(const char* text);

#endif // PLATFORMUTILS_H