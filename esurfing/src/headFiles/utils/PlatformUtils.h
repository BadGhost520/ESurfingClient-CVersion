#ifndef PLATFORMUTILS_H
#define PLATFORMUTILS_H

#include <stdint.h>

#define XML_BUFFER_SIZE 1024
#define DIALER_CONFIG_FILE "dialer.ini"

typedef enum
{
    INI_SUCCESS = 0,
    INI_FAILURE = 1,
} IniStatus;

typedef enum
{
    GET_TICKET = 1,
    LOGIN = 2,
    HEART_BEAT = 3,
    TERM = 4
} XmlChoose;

typedef enum
{
    CONSOLE_FORMAT = 1,
    FILE_FORMAT = 2
} TimeFormat;

typedef struct
{
    char usr[16];
    char pwd[128];
    char chn[8];
} IniConfig;

typedef struct
{
    unsigned char* data;
    size_t length;
} ByteArray;

/**
 * 文本转字节
 * @param str 文本数据
 * @return 字节数据
 */
ByteArray stringToBytes(const char* str);

/**
 * XML 解析
 * @param xmlData XML 数据
 * @param tag 提取标志
 * @return 解析后的数据
 */
char* XmlParser(const char* xmlData, const char* tag);

/**
 * 字符串转换为 64 位长整型
 * @param str 要转换的字符串
 * @return 转换后的 64 位长整型
 */
long long stringToLongLong(const char* str);

/**
 * 64位长整型转换为字符串
 * @param num 要转换的 64 位长整型
 * @return 转换后的字符串
 */
char* longLongToString(long long num);

/**
 * 获取当前时间的毫秒时间戳
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
 * 睡眠
 * @param milliseconds 毫秒
 */
void sleepMilliseconds(int milliseconds);

/**
 * 获取当前时间
 * @param format 格式
 * @return 格式化后的当前时间
 */
char* getTime(TimeFormat format);

/**
 * 创建 XML 字符串
 * @param choose 格式化选择
 * @return XML 字符串
 */
char* createXMLPayload(XmlChoose choose);

/**
 * 清除指定标签字段
 * @param text 需要清除的文本
 * @param start_tag 开始标签
 * @param end_tag 结束标签
 * @return 清除后的文本
 */
char* extractBetweenTags(const char* text, const char* start_tag, const char* end_tag);

/**
 * 清除 CDATA 字段
 * @param text 需要清除的文本
 * @return 清除后的文本
 */
char* cleanCDATA(const char* text);

/**
 * 创建线程函数
 * @param func 线程要执行的函数
 * @param arg 参数
 */
void createThread(void*(* func)(void*), void* arg);

/**
 * 等待线程结束函数
 * @param index 线程下标
 */
void waitThreadStop(int index);

/**
 * 获取当前线程名
 * @return 线程名
 */
const char* getThreadName();

/**
 * 保存配置文件
 */
void saveIni();

/**
 * 加载配置文件
 */
void loadIni();

#endif // PLATFORMUTILS_H