//
// Created by bad_g on 2025/9/25.
//

#ifndef ESURFINGCLIENT_UTILS_H
#define ESURFINGCLIENT_UTILS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

    // 字节数组转十六进制字符串
    char* bytes_to_hex_string(const uint8_t* bytes, size_t len);

    // 十六进制字符串转字节数组
    uint8_t* hex_string_to_bytes(const char* hex_str, size_t* len);

    // 字节序转换函数
    uint32_t swap_uint32(uint32_t val);
    uint64_t swap_uint64(uint64_t val);

    // 大端序读取32位整数
    uint32_t read_be32(const uint8_t* data);

    // 大端序写入32位整数
    void write_be32(uint8_t* data, uint32_t value);

    // 大端序读取64位整数
    uint64_t read_be64(const uint8_t* data);

    // 大端序写入64位整数
    void write_be64(uint8_t* data, uint64_t value);

    // 小端序读取32位整数
    uint32_t read_le32(const uint8_t* data);

    // 小端序写入32位整数
    void write_le32(uint8_t* data, uint32_t value);

    // 内存安全清零
    void secure_memset(void* ptr, int value, size_t num);

    // 安全的内存比较
    int secure_memcmp(const void* ptr1, const void* ptr2, size_t num);

    // PKCS7填充
    int pkcs7_pad(const uint8_t* input, size_t input_len, uint8_t** output, size_t* output_len, size_t block_size);

    // PKCS7去填充
    int pkcs7_unpad(const uint8_t* input, size_t input_len, uint8_t** output, size_t* output_len, size_t block_size);

    // 填充到指定倍数
    int pad_to_multiple(const uint8_t* input, size_t input_len, uint8_t** output, size_t* output_len, size_t multiple);

    // 去除填充
    int unpad_from_multiple(const uint8_t* input, size_t input_len, uint8_t** output, size_t* output_len);

#ifdef __cplusplus
}
#endif

#endif //ESURFINGCLIENT_UTILS_H