#include "../headFiles/cipher/cipher_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * 通用工具函数实现
 */

// 将字节数组转换为大写十六进制字符串
char* bytes_to_hex_upper(const uint8_t* bytes, size_t len) {
    if (!bytes || len == 0) return NULL;
    
    char* hex = safe_malloc(len * 2 + 1);
    if (!hex) return NULL;
    
    for (size_t i = 0; i < len; i++) {
        sprintf(hex + i * 2, "%02X", bytes[i]);
    }
    hex[len * 2] = '\0';
    return hex;
}

// 将字节数组转换为小写十六进制字符串
char* bytes_to_hex_lower(const uint8_t* bytes, size_t len) {
    if (!bytes || len == 0) return NULL;
    
    char* hex = safe_malloc(len * 2 + 1);
    if (!hex) return NULL;
    
    for (size_t i = 0; i < len; i++) {
        sprintf(hex + i * 2, "%02x", bytes[i]);
    }
    hex[len * 2] = '\0';
    return hex;
}

// 将十六进制字符串转换为字节数组
uint8_t* hex_to_bytes(const char* hex, size_t* out_len) {
    if (!hex || !out_len) return NULL;
    
    size_t hex_len = strlen(hex);
    if (hex_len % 2 != 0) return NULL; // 十六进制字符串长度必须是偶数
    
    size_t byte_len = hex_len / 2;
    uint8_t* bytes = safe_malloc(byte_len);
    if (!bytes) return NULL;
    
    for (size_t i = 0; i < byte_len; i++) {
        char byte_str[3] = {hex[i * 2], hex[i * 2 + 1], '\0'};
        bytes[i] = (uint8_t)strtol(byte_str, NULL, 16);
    }
    
    *out_len = byte_len;
    return bytes;
}

// 安全的内存分配
void* safe_malloc(size_t size) {
    if (size == 0) return NULL;
    void* ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "Memory allocation failed: %zu bytes\n", size);
        exit(EXIT_FAILURE);
    }
    return ptr;
}

// 安全的内存分配并清零
void* safe_calloc(size_t count, size_t size) {
    if (count == 0 || size == 0) return NULL;
    void* ptr = calloc(count, size);
    if (!ptr) {
        fprintf(stderr, "Memory allocation failed: %zu * %zu bytes\n", count, size);
        exit(EXIT_FAILURE);
    }
    return ptr;
}

// 安全的内存释放
void safe_free(void* ptr) {
    if (ptr) {
        free(ptr);
    }
}

// 安全的字符串复制
char* safe_strdup(const char* str) {
    if (!str) return NULL;
    
    size_t len = strlen(str);
    char* copy = safe_malloc(len + 1);
    strcpy(copy, str);
    return copy;
}

// 安全的字符串长度计算
size_t safe_strlen(const char* str) {
    return str ? strlen(str) : 0;
}

// PKCS7填充
uint8_t* pkcs7_padding(const uint8_t* data, size_t data_len, size_t block_size, size_t* out_len) {
    if (!data || !out_len || block_size == 0) return NULL;
    
    size_t padding = block_size - (data_len % block_size);
    if (padding == 0) padding = block_size;
    
    size_t padded_len = data_len + padding;
    uint8_t* padded = safe_malloc(padded_len);
    
    memcpy(padded, data, data_len);
    memset(padded + data_len, (uint8_t)padding, padding);
    
    *out_len = padded_len;
    return padded;
}

// 移除PKCS7填充
uint8_t* remove_pkcs7_padding(const uint8_t* data, size_t data_len, size_t* out_len) {
    if (!data || !out_len || data_len == 0) return NULL;
    
    uint8_t padding = data[data_len - 1];
    if (padding == 0 || padding > data_len) return NULL;
    
    // 验证填充是否正确
    for (size_t i = data_len - padding; i < data_len; i++) {
        if (data[i] != padding) return NULL;
    }
    
    size_t unpadded_len = data_len - padding;
    uint8_t* unpadded = safe_malloc(unpadded_len);
    memcpy(unpadded, data, unpadded_len);
    
    *out_len = unpadded_len;
    return unpadded;
}

// 数据对齐填充（用于XTEA等算法）
uint8_t* pad_to_multiple(const uint8_t* data, size_t data_len, size_t multiple, size_t* out_len) {
    if (!data || !out_len || multiple == 0) return NULL;
    
    size_t padding = (multiple - (data_len % multiple)) % multiple;
    size_t padded_len = data_len + padding;
    
    uint8_t* padded = safe_calloc(padded_len, 1);
    memcpy(padded, data, data_len);
    // 剩余部分已经被calloc清零
    
    *out_len = padded_len;
    return padded;
}

// 大端字节序转uint32
uint32_t bytes_to_uint32_be(const uint8_t* bytes) {
    if (!bytes) return 0;
    return ((uint32_t)bytes[0] << 24) | 
           ((uint32_t)bytes[1] << 16) | 
           ((uint32_t)bytes[2] << 8) | 
           ((uint32_t)bytes[3]);
}

// 小端字节序转uint32
uint32_t bytes_to_uint32_le(const uint8_t* bytes) {
    if (!bytes) return 0;
    return ((uint32_t)bytes[3] << 24) | 
           ((uint32_t)bytes[2] << 16) | 
           ((uint32_t)bytes[1] << 8) | 
           ((uint32_t)bytes[0]);
}

// uint32转大端字节序
void uint32_to_bytes_be(uint32_t value, uint8_t* bytes) {
    if (!bytes) return;
    bytes[0] = (uint8_t)(value >> 24);
    bytes[1] = (uint8_t)(value >> 16);
    bytes[2] = (uint8_t)(value >> 8);
    bytes[3] = (uint8_t)(value);
}

// uint32转小端字节序
void uint32_to_bytes_le(uint32_t value, uint8_t* bytes) {
    if (!bytes) return;
    bytes[0] = (uint8_t)(value);
    bytes[1] = (uint8_t)(value >> 8);
    bytes[2] = (uint8_t)(value >> 16);
    bytes[3] = (uint8_t)(value >> 24);
}

// XOR操作
void xor_bytes(const uint8_t* a, const uint8_t* b, uint8_t* result, size_t len) {
    if (!a || !b || !result) return;
    for (size_t i = 0; i < len; i++) {
        result[i] = a[i] ^ b[i];
    }
}