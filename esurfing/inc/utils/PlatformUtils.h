#ifndef PLATFORMUTILS_H
#define PLATFORMUTILS_H

#include "States.h"

#include <inttypes.h>
#include <stdint.h>

#ifdef _WIN32

#pragma comment(lib, "IPHLPAPI.lib")

#else

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <ifaddrs.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#endif

#define XML_BUFFER_SIZE 1024
#define DIALER_CONFIG_FILE "dialer.json"
#define NAME_LENGTH 128

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
    unsigned char* data;
    size_t length;
} ByteArray;

typedef struct
{
    char ip[IP_LENGTH];
    char name[NAME_LENGTH];
} Adapter;

/**
 * @brief 获取适配器数据
 */
void getAdapters();

/**
 * @brief 打包适配器数据
 * @return JSON 文本
 */
char* getAdaptersJSON();

/**
 * @brief 文本转字节
 * @param str 文本数据
 * @return 字节数据
 */
ByteArray stringToBytes(const char* str);

/**
 * @brief XML 解析
 * @param xmlData XML 数据
 * @param tag 提取标志
 * @return 解析后的数据
 */
char* XmlParser(const char* xmlData, const char* tag);

/**
 * @brief 字符串转换为 64 位长整型
 * @param str 要转换的字符串
 * @return 转换后的 64 位长整型
 */
uint64_t stringToUint64(const char* str);

/**
 * @brief 64 位长整型转换为字符串
 * @param num 要转换的 64 位长整型
 * @return 转换后的字符串
 */
char* uint64ToString(uint64_t num);

/**
 * @brief 获取当前时间的毫秒时间戳
 * @return 64位时间戳
 */
uint64_t currentTimeMillis();

/**
 * @brief 获取随机字节
 * @param buffer 缓冲
 * @param length 长度
 */
void randomBytes(unsigned char* buffer, size_t length);

/**
 * @brief 睡眠
 * @param milliseconds 毫秒
 */
void sleepMilliseconds(uint64_t milliseconds);

/**
 * @brief 获取当前时间
 * @param format 格式
 * @return 格式化后的当前时间
 */
char* getTime(TimeFormat format);

/**
 * @brief 创建 XML 字符串
 * @param choose 格式化选择
 * @return XML 字符串
 */
char* createXMLPayload(XmlChoose choose);

/**
 * @brief 清除指定标签字段
 * @param text 需要清除的文本
 * @param start_tag 开始标签
 * @param end_tag 结束标签
 * @return 清除后的文本
 */
char* extractBetweenTags(const char* text, const char* start_tag, const char* end_tag);

/**
 * @brief 清除 CDATA 字段
 * @param text 需要清除的文本
 * @return 清除后的文本
 */
char* cleanCDATA(const char* text);

/**
 * @brief 保存配置文件
 */
void saveJSON();

/**
 * @brief 加载配置文件
 */
void loadJSON();

#endif // PLATFORMUTILS_H