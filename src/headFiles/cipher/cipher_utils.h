#ifndef CIPHER_UTILS_H
#define CIPHER_UTILS_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 通用工具函数 - 用于加解密算法的辅助功能
 */

// 十六进制字符串转换
char* bytes_to_hex_upper(const uint8_t* bytes, size_t len);
char* bytes_to_hex_lower(const uint8_t* bytes, size_t len);
uint8_t* hex_to_bytes(const char* hex, size_t* out_len);

// 内存操作
void* safe_malloc(size_t size);
void* safe_calloc(size_t count, size_t size);
void safe_free(void* ptr);

// 字符串操作
char* safe_strdup(const char* str);
size_t safe_strlen(const char* str);

// 数据填充
uint8_t* pkcs7_padding(const uint8_t* data, size_t data_len, size_t block_size, size_t* out_len);
uint8_t* remove_pkcs7_padding(const uint8_t* data, size_t data_len, size_t* out_len);

// 数据对齐填充（用于XTEA等算法）
uint8_t* pad_to_multiple(const uint8_t* data, size_t data_len, size_t multiple, size_t* out_len);

// 字节序转换
uint32_t bytes_to_uint32_be(const uint8_t* bytes);
uint32_t bytes_to_uint32_le(const uint8_t* bytes);
void uint32_to_bytes_be(uint32_t value, uint8_t* bytes);
void uint32_to_bytes_le(uint32_t value, uint8_t* bytes);

// XOR操作
void xor_bytes(const uint8_t* a, const uint8_t* b, uint8_t* result, size_t len);

#ifdef __cplusplus
}
#endif

#endif // CIPHER_UTILS_H