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
 * @param tag 提取标志
 * @param parsed 解析后储存的指针地址
 */
void XmlParser(const char* xmlData, const char* tag, char** parsed);

/**
 * 字符串转换为64位长整型函数
 * @param str 要转换的字符串
 * @param result 转换结果的指针
 * @return 是否成功
 */
int stringToLongLong(const char* str, long long* result);

/**
 * 64位长整型转换为字符串函数
 * @param string 转换后要储存到的指针地址
 * @param num 要转换的64位长整型
 */
void longLongToString(char** string, long long num);

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
 * @param timestamp 要储存到的指针地址
 */
void getTime(char** timestamp);

/**
 * 获取当前时间函数(文件)
 * @param timestamp 要储存到的指针地址
 */
void getFileTime(char** timestamp);

/**
 * 创建 XML 字符串函数
 * @param choose 格式化选择
 * @param payload 创建 xml 后储存的指针地址
 */
void createXMLPayload(XmlChoose choose, char** payload);

/**
 * 清除 CDATA 字段函数
 * @param text 未清除 CDATA 字段文本
 * @param cleaned 清除后要储存到的指针地址
 */
void cleanCDATA(const char* text, char** cleaned);

/**
 * 创建线程函数
 * @param func 线程要执行的函数
 * @param arg 参数
 * @param index 线程下标
 */
void createThread(void*(* func)(void*), void* arg, int index);

/**
 * 等待线程结束函数
 * @param index 线程下标
 */
void waitThreadStop(int index);

#endif // PLATFORMUTILS_H