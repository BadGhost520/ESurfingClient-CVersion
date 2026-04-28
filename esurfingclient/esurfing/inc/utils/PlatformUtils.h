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
#include <net/if.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#endif

#define XML_BUFFER_SIZE 1024
#define DIALER_CONFIG_FILE "ESurfingClient.json"
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
    uint8_t* data;
    size_t length;
} bytes_t;

/**
 * @brief XML 解析
 * @param xml_data XML 数据
 * @param tag 提取标志
 * @return 解析后的数据
 */
char* xml_parser(const char* xml_data, const char* tag);

/**
 * @brief 文本转字节
 * @param str 文本数据
 * @return 字节数据
 */
bytes_t str_2_bytes(const char* str);

/**
 * @brief 字符串转换为 64 位长整型
 * @param str 要转换的字符串
 * @return 转换后的 64 位长整型
 */
uint64_t str_2_uint64(const char* str);

/**
 * @brief 64 位长整型转换为字符串
 * @param num 要转换的 64 位长整型
 * @return 转换后的字符串
 */
char* uint64_2_str(uint64_t num);

/**
 * @brief 获取当前时间的毫秒时间戳
 * @return 64位时间戳
 */
uint64_t get_cur_tm_ms();

/**
 * @brief 获取随机字节
 * @param buf 缓冲
 * @param len 长度
 */
void get_rand_bytes(uint8_t* buf, size_t len);

/**
 * @brief 睡眠
 * @param ms 毫秒
 */
void sleep_ms(uint64_t ms);

/**
 * @brief 获取当前时间
 * @param buf 时间戳缓冲区
 * @param fmt 格式
 */
void get_fmt_time(char* buf, TimeFormat fmt);

/**
 * @brief 安全字符串
 * @param str 字符串
 * @return 过滤后的字符串
 */
const char* safe_str(const char* str);

/**
 * @brief 创建 XML 字符串
 * @param choose 格式化选择
 * @return XML 字符串
 */
char* create_xml_payload(XmlChoose choose);

/**
 * @brief 清除指定标签字段
 * @param text 需要清除的文本
 * @param start_tag 开始标签
 * @param end_tag 结束标签
 * @return 清除后的文本
 */
char* extract_between_tags(const char* text, const char* start_tag, const char* end_tag);

/**
 * @brief 清除 CDATA 字段
 * @param text 需要清除的文本
 * @return 清除后的文本
 */
char* clean_CDATA(const char* text);

/**
 * @brief 保存配置文件
 */
// bool save_cfg();

/**
 * @brief 加载配置文件
 */
bool load_cfg();

#endif // PLATFORMUTILS_H
