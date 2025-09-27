#include "../../headFiles/cipher/cipher_interface.h"
#include "../../headFiles/cipher/cipher_utils.h"
#include <string.h>
#include <stdint.h>

/**
 * ModXTEA实现 - 对应Kotlin的ModXTEA类
 * 
 * ModXTEA是修改版的XTEA算法，使用三轮加密：
 * 1. 使用key1进行第一轮XTEA加密
 * 2. 使用key2进行第二轮XTEA加密
 * 3. 使用key3进行第三轮XTEA加密
 */

#define XTEA_NUM_ROUNDS 32
#define XTEA_DELTA 0x9E3779B9

typedef struct {
    uint32_t key1[4];
    uint32_t key2[4];
    uint32_t key3[4];
} mod_xtea_data_t;

// 从字节数组中读取32位整数（大端序）
static uint32_t get_uint32_be(const uint8_t* data, size_t offset) {
    return (data[offset] << 24) | (data[offset + 1] << 16) |
           (data[offset + 2] << 8) | data[offset + 3];
}

// 将32位整数写入字节数组（大端序）
static void set_uint32_be(uint8_t* data, size_t offset, uint32_t value) {
    data[offset] = (value >> 24) & 0xFF;
    data[offset + 1] = (value >> 16) & 0xFF;
    data[offset + 2] = (value >> 8) & 0xFF;
    data[offset + 3] = value & 0xFF;
}

// XTEA加密一个块（8字节）
static void xtea_encrypt_block(uint32_t* v0, uint32_t* v1, const uint32_t* key) {
    uint32_t sum = 0;
    
    for (int i = 0; i < XTEA_NUM_ROUNDS; i++) {
        *v0 += ((*v1 ^ sum) + key[sum & 3]) + ((*v1 << 4) ^ (*v1 >> 5));
        sum += XTEA_DELTA;
        *v1 += key[(sum >> 11) & 3] + (*v0 ^ sum) + ((*v0 << 4) ^ (*v0 >> 5));
    }
}

// XTEA解密一个块（8字节）
static void xtea_decrypt_block(uint32_t* v0, uint32_t* v1, const uint32_t* key) {
    uint32_t sum = XTEA_DELTA * XTEA_NUM_ROUNDS;
    
    for (int i = 0; i < XTEA_NUM_ROUNDS; i++) {
        *v1 -= key[(sum >> 11) & 3] + (*v0 ^ sum) + ((*v0 << 4) ^ (*v0 >> 5));
        sum -= XTEA_DELTA;
        *v0 -= ((*v1 ^ sum) + key[sum & 3]) + ((*v1 << 4) ^ (*v1 >> 5));
    }
}

// 加密实现 - 对应Kotlin: override fun encrypt(text: String): String
static char* mod_xtea_encrypt(cipher_interface_t* self, const char* text) {
    if (!self || !text) return NULL;
    
    mod_xtea_data_t* data = (mod_xtea_data_t*)self->private_data;
    if (!data) return NULL;
    
    size_t text_len = strlen(text);
    
    // 8字节对齐填充
    size_t padded_len;
    uint8_t* padded_data = pad_to_multiple((const uint8_t*)text, text_len, 8, &padded_len);
    if (!padded_data) return NULL;
    
    // 分配输出缓冲区
    uint8_t* output = safe_malloc(padded_len);
    memcpy(output, padded_data, padded_len);
    safe_free(padded_data);
    
    // 逐块处理（每块8字节）
    for (size_t i = 0; i < padded_len; i += 8) {
        uint32_t v0 = get_uint32_be(output, i);
        uint32_t v1 = get_uint32_be(output, i + 4);
        
        // 第一轮加密（使用key1）
        xtea_encrypt_block(&v0, &v1, data->key1);
        
        // 第二轮加密（使用key2）
        xtea_encrypt_block(&v0, &v1, data->key2);
        
        // 第三轮加密（使用key3）
        xtea_encrypt_block(&v0, &v1, data->key3);
        
        // 写回结果
        set_uint32_be(output, i, v0);
        set_uint32_be(output, i + 4, v1);
    }
    
    // 转换为大写十六进制字符串
    char* hex_result = bytes_to_hex_upper(output, padded_len);
    safe_free(output);
    
    return hex_result;
}

// 解密实现 - 对应Kotlin: override fun decrypt(hex: String): String
static char* mod_xtea_decrypt(cipher_interface_t* self, const char* hex) {
    if (!self || !hex) return NULL;
    
    mod_xtea_data_t* data = (mod_xtea_data_t*)self->private_data;
    if (!data) return NULL;
    
    // 将十六进制字符串转换为字节数组
    size_t bytes_len;
    uint8_t* bytes = hex_to_bytes(hex, &bytes_len);
    if (!bytes) return NULL;
    
    // 分配输出缓冲区
    uint8_t* output = safe_malloc(bytes_len);
    memcpy(output, bytes, bytes_len);
    safe_free(bytes);
    
    // 逐块处理（每块8字节）
    for (size_t i = 0; i < bytes_len; i += 8) {
        uint32_t v0 = get_uint32_be(output, i);
        uint32_t v1 = get_uint32_be(output, i + 4);
        
        // 第一轮解密（使用key3，逆序）
        xtea_decrypt_block(&v0, &v1, data->key3);
        
        // 第二轮解密（使用key2）
        xtea_decrypt_block(&v0, &v1, data->key2);
        
        // 第三轮解密（使用key1）
        xtea_decrypt_block(&v0, &v1, data->key1);
        
        // 写回结果
        set_uint32_be(output, i, v0);
        set_uint32_be(output, i + 4, v1);
    }
    
    // 移除尾部的零字节填充（对应Kotlin中的dropLastWhile）
    while (bytes_len > 0 && output[bytes_len - 1] == 0) {
        bytes_len--;
    }
    
    // 转换为字符串
    char* result = safe_malloc(bytes_len + 1);
    memcpy(result, output, bytes_len);
    result[bytes_len] = '\0';
    safe_free(output);
    
    return result;
}

// 销毁函数
static void mod_xtea_destroy(cipher_interface_t* self) {
    if (self) {
        safe_free(self->private_data);
        safe_free(self);
    }
}

// 创建ModXTEA加解密实例
cipher_interface_t* create_mod_xtea_cipher(const uint32_t* key1, const uint32_t* key2, 
                                           const uint32_t* key3) {
    if (!key1 || !key2 || !key3) return NULL;
    
    cipher_interface_t* cipher = safe_malloc(sizeof(cipher_interface_t));
    mod_xtea_data_t* data = safe_malloc(sizeof(mod_xtea_data_t));
    
    // 复制密钥
    memcpy(data->key1, key1, 4 * sizeof(uint32_t));
    memcpy(data->key2, key2, 4 * sizeof(uint32_t));
    memcpy(data->key3, key3, 4 * sizeof(uint32_t));
    
    // 设置函数指针
    cipher->encrypt = mod_xtea_encrypt;
    cipher->decrypt = mod_xtea_decrypt;
    cipher->destroy = mod_xtea_destroy;
    cipher->private_data = data;
    
    return cipher;
}