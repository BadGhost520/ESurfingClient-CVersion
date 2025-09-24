//
// Created by bad_g on 2025/9/25.
//
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "../headFiles/cipher/Utils.h"

// 字节数组转十六进制字符串
char* bytes_to_hex_string(const uint8_t* bytes, size_t len) {
    if (!bytes || len == 0) return NULL;

    char* hex_str = malloc(len * 2 + 1);
    if (!hex_str) return NULL;

    for (size_t i = 0; i < len; i++) {
        sprintf(hex_str + i * 2, "%02X", bytes[i]);
    }
    hex_str[len * 2] = '\0';

    return hex_str;
}

// 十六进制字符串转字节数组
uint8_t* hex_string_to_bytes(const char* hex_str, size_t* len) {
    if (!hex_str || !len) return NULL;

    size_t hex_len = strlen(hex_str);
    if (hex_len % 2 != 0) return NULL;

    *len = hex_len / 2;
    uint8_t* bytes = malloc(*len);
    if (!bytes) return NULL;

    for (size_t i = 0; i < *len; i++) {
        char hex_pair[3] = {hex_str[i * 2], hex_str[i * 2 + 1], '\0'};
        char* endptr;
        unsigned long byte_val = strtoul(hex_pair, &endptr, 16);

        // 检查转换是否成功
        if (endptr != hex_pair + 2 || byte_val > 255) {
            free(bytes);
            return NULL;
        }
        bytes[i] = (uint8_t)byte_val;
    }

    return bytes;
}

// 字节序转换函数
uint32_t swap_uint32(uint32_t val) {
    return ((val & 0xFF000000) >> 24) |
           ((val & 0x00FF0000) >> 8)  |
           ((val & 0x0000FF00) << 8)  |
           ((val & 0x000000FF) << 24);
}

uint64_t swap_uint64(uint64_t val) {
    return ((val & 0xFF00000000000000ULL) >> 56) |
           ((val & 0x00FF000000000000ULL) >> 40) |
           ((val & 0x0000FF0000000000ULL) >> 24) |
           ((val & 0x000000FF00000000ULL) >> 8)  |
           ((val & 0x00000000FF000000ULL) << 8)  |
           ((val & 0x0000000000FF0000ULL) << 24) |
           ((val & 0x000000000000FF00ULL) << 40) |
           ((val & 0x00000000000000FFULL) << 56);
}

// 大端序读取32位整数
uint32_t read_be32(const uint8_t* data) {
    return ((uint32_t)data[0] << 24) |
           ((uint32_t)data[1] << 16) |
           ((uint32_t)data[2] << 8)  |
           ((uint32_t)data[3]);
}

// 大端序写入32位整数
void write_be32(uint8_t* data, uint32_t value) {
    data[0] = (uint8_t)(value >> 24);
    data[1] = (uint8_t)(value >> 16);
    data[2] = (uint8_t)(value >> 8);
    data[3] = (uint8_t)(value);
}

// 大端序读取64位整数
uint64_t read_be64(const uint8_t* data) {
    return ((uint64_t)data[0] << 56) |
           ((uint64_t)data[1] << 48) |
           ((uint64_t)data[2] << 40) |
           ((uint64_t)data[3] << 32) |
           ((uint64_t)data[4] << 24) |
           ((uint64_t)data[5] << 16) |
           ((uint64_t)data[6] << 8)  |
           ((uint64_t)data[7]);
}

// 大端序写入64位整数
void write_be64(uint8_t* data, uint64_t value) {
    data[0] = (uint8_t)(value >> 56);
    data[1] = (uint8_t)(value >> 48);
    data[2] = (uint8_t)(value >> 40);
    data[3] = (uint8_t)(value >> 32);
    data[4] = (uint8_t)(value >> 24);
    data[5] = (uint8_t)(value >> 16);
    data[6] = (uint8_t)(value >> 8);
    data[7] = (uint8_t)(value);
}

// 小端序读取32位整数
uint32_t read_le32(const uint8_t* data) {
    return ((uint32_t)data[3] << 24) |
           ((uint32_t)data[2] << 16) |
           ((uint32_t)data[1] << 8)  |
           ((uint32_t)data[0]);
}

// 小端序写入32位整数
void write_le32(uint8_t* data, uint32_t value) {
    data[0] = (uint8_t)(value);
    data[1] = (uint8_t)(value >> 8);
    data[2] = (uint8_t)(value >> 16);
    data[3] = (uint8_t)(value >> 24);
}

// 内存安全清零
void secure_memset(void* ptr, int value, size_t num) {
    volatile uint8_t* p = (volatile uint8_t*)ptr;
    while (num--) {
        *p++ = (uint8_t)value;
    }
}

// 安全的内存比较
int secure_memcmp(const void* ptr1, const void* ptr2, size_t num) {
    const uint8_t* p1 = (const uint8_t*)ptr1;
    const uint8_t* p2 = (const uint8_t*)ptr2;
    int result = 0;

    for (size_t i = 0; i < num; i++) {
        result |= p1[i] ^ p2[i];
    }

    return result;
}

// PKCS7填充
int pkcs7_pad(const uint8_t* input, size_t input_len, uint8_t** output, size_t* output_len, size_t block_size) {
    if (!input || !output || !output_len || block_size == 0 || block_size > 255) {
        return -1;
    }

    size_t padding_len = block_size - (input_len % block_size);
    *output_len = input_len + padding_len;

    *output = malloc(*output_len);
    if (!*output) return -1;

    memcpy(*output, input, input_len);
    memset(*output + input_len, (int)padding_len, padding_len);

    return 0;
}

// PKCS7去填充
int pkcs7_unpad(const uint8_t* input, size_t input_len, uint8_t** output, size_t* output_len, size_t block_size) {
    if (!input || !output || !output_len || input_len == 0 || input_len % block_size != 0) {
        return -1;
    }

    uint8_t padding_len = input[input_len - 1];
    if (padding_len == 0 || padding_len > block_size || padding_len > input_len) {
        return -1;
    }

    // 验证填充
    for (size_t i = input_len - padding_len; i < input_len; i++) {
        if (input[i] != padding_len) {
            return -1;
        }
    }

    *output_len = input_len - padding_len;
    *output = malloc(*output_len);
    if (!*output) return -1;

    memcpy(*output, input, *output_len);

    return 0;
}

// 填充到指定倍数
int pad_to_multiple(const uint8_t* input, size_t input_len, uint8_t** output, size_t* output_len, size_t multiple) {
    if (!input || !output || !output_len || multiple == 0) {
        return -1;
    }

    size_t remainder = input_len % multiple;
    if (remainder == 0) {
        *output_len = input_len;
        *output = malloc(*output_len);
        if (!*output) return -1;
        memcpy(*output, input, input_len);
        return 0;
    }

    size_t padding_len = multiple - remainder;
    *output_len = input_len + padding_len;

    *output = malloc(*output_len);
    if (!*output) return -1;

    memcpy(*output, input, input_len);
    memset(*output + input_len, 0, padding_len);

    return 0;
}

// 去除填充
int unpad_from_multiple(const uint8_t* input, size_t input_len, uint8_t** output, size_t* output_len) {
    if (!input || !output || !output_len || input_len == 0) {
        return -1;
    }

    // 找到最后一个非零字节
    size_t actual_len = input_len;
    while (actual_len > 0 && input[actual_len - 1] == 0) {
        actual_len--;
    }

    *output_len = actual_len;
    *output = malloc(*output_len);
    if (!*output) return -1;

    memcpy(*output, input, *output_len);

    return 0;
}