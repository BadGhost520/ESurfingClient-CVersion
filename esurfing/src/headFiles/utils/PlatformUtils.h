#ifndef PLATFORMUTILS_H
#define PLATFORMUTILS_H

#include <stdint.h>

#include "../States.h"

typedef struct {
    unsigned char* data;
    size_t length;
} ByteArray;

typedef enum {
    GetTicket = 1,
    Login = 2,
    Heartbeat = 3,
    Term = 4
} XmlChoose;

/**
 * 文本转字节函数
 * @param str 文本数据
 * @return 字节数据
 */
ByteArray stringToBytes(const char* str);

/**
 * XML 解析函数
 * @param xmlData XML 数据
 * @param tag 标志
 * @return 解析后的数据
 */
char* XmlParser(const char* xmlData, const char* tag);

/**
 * 字符串转换为64位长整型函数
 * @param str 要转换的字符串
 * @param result 转换结果的指针
 * @return 是否成功
 */
int stringToLongLong(const char* str, long long* result);

/**
 * 64位长整型转换为字符串函数
 * @param num 要转换的64位长整型
 * @return 转换后的字符串
 */
char* longLongToString(long long num);

/**
 * 获取当前时间的毫秒时间戳函数
 * @return 64位时间戳
 */
int64_t currentTimeMillis();

/**
 * 获取随机字节
 * @param buffer 缓冲
 * @param length 长度
 * @return 随机字节
 */
int randomBytes(unsigned char* buffer, size_t length);

/**
 * 睡眠函数
 * @param milliseconds 毫秒
 */
void sleepMilliseconds(int milliseconds);

/**
 * 获取当前时间函数(终端)
 */
void getTime(char** timestamp);

/**
 * 获取当前时间函数(文件)
 */
void getFileTime(char** timestamp);

/**
 * 创建 XML 字符串函数
 * @param payload xml 指针
 * @param choose 格式化选择
 * @param adapter 适配器配置指针
 */
void createXMLPayload(char** payload, XmlChoose choose, DialerContext adapter);

/**
 * 清除 CDATA 字段函数
 * @param text 未清除 CDATA 字段文本
 * @return 清除 CDATA 字段之后的文本
 */
char* cleanCDATA(const char* text);

/**
 * OpenWrt 创建一键配置脚本
 */
void createBash();

/**
 * 创建线程函数
 * @param adapter 适配器配置指针
 * @param func 线程要执行的函数
 * @param arg 参数
 */
void createThread(DialerContext* adapter, void*(* func)(void*), void* arg);

/**
 * 等待线程结束函数
 * @param adapter 适配器配置指针
 */
void waitThreadStop(DialerContext adapter);

#endif // PLATFORMUTILS_H