//
// PlatformUtils.h - 跨平台工具函数
// Created by bad_g on 2025/9/14.
//

#ifndef PLATFORMUTILS_H
#define PLATFORMUTILS_H

#include <stdint.h>

/**
 * 将字符串转换为64位长整型
 * @param str 要转换的字符串
 * @param result 转换结果的指针
 * @return 转换是否成功 (1=成功, 0=失败)
 */
int stringToLongLong(const char* str, long long* result);

/**
 * 获取当前时间的毫秒时间戳函数
 * @return 64位时间戳
 */
int64_t currentTimeMillis();

/**
 * 睡眠函数
 * @param milliseconds 毫秒
 */
void sleepMilliseconds(int milliseconds);

/**
 * 设置客户端 ID 函数
 * @param client_id 客户端 ID
 */
void setClientId(char** client_id);

/**
 * 生成随机 MAC 地址函数
 * @return MAC 地址
 */
char* randomMacAddress();

/**
 * 生成10位随机字符串函数
 * @return 10位随机字符串
 */
char* randomString();

/**
 * 获取当前时间函数(终端)
 * @return 当前时间(YY-mm-dd HH-MM-SS)
 */
char* getTime();

/**
 * 获取当前时间函数(文件)
 * @return 当前时间(YYmmdd-HHMMSS)
 */
char* getFileTime();

/**
 * 创建 XML 字符串函数
 * @param choose 格式化选择
 * @return XML 字符串
 */
char* createXMLPayload(const char* choose);

/**
 * 清除 CDATA 字段函数
 * @param text 未清除 CDATA 字段文本
 * @return 清除 CDATA 字段之后的文本
 */
char* cleanCDATA(const char* text);

#endif // PLATFORMUTILS_H