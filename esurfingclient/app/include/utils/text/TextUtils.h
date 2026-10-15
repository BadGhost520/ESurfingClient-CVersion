#ifndef ESURFINGCLIENT_TEXTUTILS_H
#define ESURFINGCLIENT_TEXTUTILS_H

#include "core/States.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define XML_BUFFER_SIZE 1024

/* 文本与字节: 字符串兜底、URL 参数、base64、随机数、门户 XML */

/**
 * @brief 截取 URL 中指定参数
 * @param url URL 地址
 * @param search_str_start 要查找的参数名
 * @return 查找到的参数
 */
char* extract_url_param(const char* url, const char* search_str_start);

/**
 * @brief 安全字符串
 * @param str 字符串
 * @return 过滤后的字符串
 */
const char* safe_str(const char* str);

/**
 * @brief 字节转 base64
 * @param in 字节数据
 * @param len 字节长度
 * @return base64 码
 */
char* bytes2base64(const uint8_t* in, size_t len);

/**
 * @brief base64 转字节
 * @param in base64 码
 * @param out_len 字节长度指针
 * @return 字节数据
 */
uint8_t* base642bytes(const char* in, size_t* out_len);

/**
 * @brief 文本转字节
 * @param str 文本数据
 * @return 字节数据
 */
bytes_t str2bytes(const char* str);

/**
 * @brief 字符串转换为 64 位长整型
 * @param str 要转换的字符串
 * @return 转换后的 64 位长整型
 */
uint64_t str2uint64(const char* str);

/**
 * @brief 64 位长整型转换为字符串
 * @param num 要转换的 64 位长整型
 * @return 转换后的字符串
 */
char* uint642str(uint64_t num);

/**
 * @brief 获取随机字节
 * @param buf 缓冲
 * @param len 长度
 */
void get_rand_bytes(uint8_t* buf, size_t len);

/**
 * @brief XML 解析
 * @param xml_data XML 数据
 * @param tag 提取标志
 * @return 解析后的数据
 */
char* xml_parser(const char* xml_data, const char* tag);

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

#endif
