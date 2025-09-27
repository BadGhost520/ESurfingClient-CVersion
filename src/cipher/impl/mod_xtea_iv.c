#include "../../headFiles/cipher/cipher_interface.h"
#include "../../headFiles/cipher/cipher_utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/**
 * ModXTEAIV实现 - 对应Kotlin的ModXTEAIV类
 * 
 * ModXTEAIV是带初始化向量的修改版XTEA算法，类似CBC模式：
 * 1. 每个块先与前一个密文块（或IV）异或
 * 2. 然后进行三轮XTEA加密（key3 -> key2 -> key1）
 * 3. 解密时顺序相反（key1 -> key2 -> key3），然后与前一个密文块异或
 */

#define XTEA_NUM_ROUNDS 32
#define XTEA_DELTA 0x9E3779B9

typedef struct {
    uint32_t key1[4];
    uint32_t key2[4];
    uint32_t key3[4];
    uint32_t iv[2];  // IV为两个32位整数（8字节）
} mod_xtea_iv_data_t;

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

// 块异或操作
static void xor_block(uint32_t* v0, uint32_t* v1, const uint32_t* prev) {
    *v0 ^= prev[0];
    *v1 ^= prev[1];
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
static char* mod_xtea_iv_encrypt(cipher_interface_t* self, const char* text) {
    if (!self || !text) return NULL;
    
    mod_xtea_iv_data_t* data = (mod_xtea_iv_data_t*)self->private_data;
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
    
    // 初始化前一个块为IV
    uint32_t previous[2];
    previous[0] = data->iv[0];
    previous[1] = data->iv[1];
    
    // 逐块处理（每块8字节）
    for (size_t i = 0; i < padded_len; i += 8) {
        uint32_t v0 = get_uint32_be(output, i);
        uint32_t v1 = get_uint32_be(output, i + 4);
        
        // 与前一个块异或
        xor_block(&v0, &v1, previous);
        
        // 第一轮加密（使用key3）
        xtea_encrypt_block(&v0, &v1, data->key3);
        
        // 第二轮加密（使用key2）
        xtea_encrypt_block(&v0, &v1, data->key2);
        
        // 第三轮加密（使用key1）
        xtea_encrypt_block(&v0, &v1, data->key1);
        
        // 写回结果
        set_uint32_be(output, i, v0);
        set_uint32_be(output, i + 4, v1);
        
        // 更新前一个块为当前密文块
        previous[0] = v0;
        previous[1] = v1;
    }
    
    // 转换为大写十六进制字符串
    char* hex_result = bytes_to_hex_upper(output, padded_len);
    safe_free(output);
    
    return hex_result;
}

// 解密实现 - 对应Kotlin: override fun decrypt(hex: String): String
static char* mod_xtea_iv_decrypt(cipher_interface_t* self, const char* hex) {
    if (!self || !hex) return NULL;
    
    mod_xtea_iv_data_t* data = (mod_xtea_iv_data_t*)self->private_data;
    if (!data) return NULL;
    
    // 将十六进制字符串转换为字节数组
    size_t bytes_len;
    uint8_t* bytes = hex_to_bytes(hex, &bytes_len);
    if (!bytes) return NULL;
    
    // 分配输出缓冲区
    uint8_t* output = safe_malloc(bytes_len);
    memcpy(output, bytes, bytes_len);
    
    // 初始化前一个块为IV
    uint32_t previous[2];
    previous[0] = data->iv[0];
    previous[1] = data->iv[1];
    
    // 逐块处理（每块8字节）
    for (size_t i = 0; i < bytes_len; i += 8) {
        uint32_t v0 = get_uint32_be(output, i);
        uint32_t v1 = get_uint32_be(output, i + 4);
        
        // 保存当前密文块
        uint32_t cipher_block[2] = {v0, v1};
        
        // 第一轮解密（使用key1）
        xtea_decrypt_block(&v0, &v1, data->key1);
        
        // 第二轮解密（使用key2）
        xtea_decrypt_block(&v0, &v1, data->key2);
        
        // 第三轮解密（使用key3）
        xtea_decrypt_block(&v0, &v1, data->key3);
        
        // 与前一个密文块异或
        xor_block(&v0, &v1, previous);
        
        // 写回结果
        set_uint32_be(output, i, v0);
        set_uint32_be(output, i + 4, v1);
        
        // 更新前一个块为当前密文块
        previous[0] = cipher_block[0];
        previous[1] = cipher_block[1];
    }
    
    safe_free(bytes);
    
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
static void mod_xtea_iv_destroy(cipher_interface_t* self) {
    if (self) {
        safe_free(self->private_data);
        safe_free(self);
    }
}

// 创建ModXTEAIV加解密实例
cipher_interface_t* create_mod_xtea_iv_cipher(const uint32_t* key1, const uint32_t* key2, 
                                              const uint32_t* key3, const uint32_t* iv) {
    if (!key1 || !key2 || !key3 || !iv) return NULL;
    
    cipher_interface_t* cipher = safe_malloc(sizeof(cipher_interface_t));
    mod_xtea_iv_data_t* data = safe_malloc(sizeof(mod_xtea_iv_data_t));
    
    // 复制密钥和IV
    memcpy(data->key1, key1, 4 * sizeof(uint32_t));
    memcpy(data->key2, key2, 4 * sizeof(uint32_t));
    memcpy(data->key3, key3, 4 * sizeof(uint32_t));
    memcpy(data->iv, iv, 2 * sizeof(uint32_t));
    
    // 设置函数指针
    cipher->encrypt = mod_xtea_iv_encrypt;
    cipher->decrypt = mod_xtea_iv_decrypt;
    cipher->destroy = mod_xtea_iv_destroy;
    cipher->private_data = data;
    
    return cipher;
}