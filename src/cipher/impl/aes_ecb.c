#include "../../headFiles/cipher/cipher_interface.h"
#include "../../headFiles/cipher/cipher_utils.h"
#include <openssl/evp.h>
#include <stdio.h>
#include <string.h>

/**
 * AES-ECB实现 - 对应Kotlin的AESECB类
 * 
 * 实现双重AES-ECB加密：
 * 1. 使用key1进行第一次AES-ECB加密
 * 2. 使用key2进行第二次AES-ECB加密
 */

typedef struct {
    uint8_t key1[16];
    uint8_t key2[16];
} aes_ecb_data_t;

// AES-ECB加密函数
static uint8_t* aes_encrypt_ecb(const uint8_t* data, size_t data_len, 
                                const uint8_t* key, size_t* out_len) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return NULL;
    
    // 初始化加密上下文
    if (EVP_EncryptInit_ex(ctx, EVP_aes_128_ecb(), NULL, key, NULL) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return NULL;
    }
    
    // 禁用自动填充，我们手动处理填充
    EVP_CIPHER_CTX_set_padding(ctx, 0);
    
    // 手动进行16字节对齐填充
    size_t padded_len;
    uint8_t* padded_data = pad_to_multiple(data, data_len, 16, &padded_len);
    if (!padded_data) {
        EVP_CIPHER_CTX_free(ctx);
        return NULL;
    }
    
    // 分配输出缓冲区
    uint8_t* output = safe_malloc(padded_len);
    
    int len;
    int ciphertext_len = 0;
    
    // 执行加密
    if (EVP_EncryptUpdate(ctx, output, &len, padded_data, padded_len) != 1) {
        safe_free(padded_data);
        safe_free(output);
        EVP_CIPHER_CTX_free(ctx);
        return NULL;
    }
    ciphertext_len = len;
    
    // 完成加密
    if (EVP_EncryptFinal_ex(ctx, output + len, &len) != 1) {
        safe_free(padded_data);
        safe_free(output);
        EVP_CIPHER_CTX_free(ctx);
        return NULL;
    }
    ciphertext_len += len;
    
    safe_free(padded_data);
    EVP_CIPHER_CTX_free(ctx);
    
    *out_len = ciphertext_len;
    return output;
}

// AES-ECB解密函数
static uint8_t* aes_decrypt_ecb(const uint8_t* data, size_t data_len, 
                                const uint8_t* key, size_t* out_len) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return NULL;
    
    // 初始化解密上下文
    if (EVP_DecryptInit_ex(ctx, EVP_aes_128_ecb(), NULL, key, NULL) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return NULL;
    }
    
    // 禁用自动填充
    EVP_CIPHER_CTX_set_padding(ctx, 0);
    
    // 分配输出缓冲区
    uint8_t* output = safe_malloc(data_len);
    
    int len;
    int plaintext_len = 0;
    
    // 执行解密
    if (EVP_DecryptUpdate(ctx, output, &len, data, data_len) != 1) {
        safe_free(output);
        EVP_CIPHER_CTX_free(ctx);
        return NULL;
    }
    plaintext_len = len;
    
    // 完成解密
    if (EVP_DecryptFinal_ex(ctx, output + len, &len) != 1) {
        safe_free(output);
        EVP_CIPHER_CTX_free(ctx);
        return NULL;
    }
    plaintext_len += len;
    
    EVP_CIPHER_CTX_free(ctx);
    
    *out_len = plaintext_len;
    return output;
}

// 加密实现 - 对应Kotlin: override fun encrypt(text: String): String
static char* aes_ecb_encrypt(cipher_interface_t* self, const char* text) {
    if (!self || !text) return NULL;
    
    aes_ecb_data_t* data = (aes_ecb_data_t*)self->private_data;
    if (!data) return NULL;
    
    size_t text_len = strlen(text);
    
    // 第一次AES-ECB加密
    size_t r1_len;
    uint8_t* r1 = aes_encrypt_ecb((const uint8_t*)text, text_len, 
                                  data->key1, &r1_len);
    if (!r1) return NULL;
    
    // 第二次AES-ECB加密
    size_t r2_len;
    uint8_t* r2 = aes_encrypt_ecb(r1, r1_len, data->key2, &r2_len);
    safe_free(r1);
    
    if (!r2) return NULL;
    
    // 转换为大写十六进制字符串
    char* hex_result = bytes_to_hex_upper(r2, r2_len);
    safe_free(r2);
    
    return hex_result;
}

// 解密实现 - 对应Kotlin: override fun decrypt(hex: String): String
static char* aes_ecb_decrypt(cipher_interface_t* self, const char* hex) {
    if (!self || !hex) return NULL;
    
    aes_ecb_data_t* data = (aes_ecb_data_t*)self->private_data;
    if (!data) return NULL;
    
    // 将十六进制字符串转换为字节数组
    size_t bytes_len;
    uint8_t* bytes = hex_to_bytes(hex, &bytes_len);
    if (!bytes) return NULL;
    
    // 第一次解密（使用key2）
    size_t r1_len;
    uint8_t* r1 = aes_decrypt_ecb(bytes, bytes_len, data->key2, &r1_len);
    safe_free(bytes);
    
    if (!r1) return NULL;
    
    // 第二次解密（使用key1）
    size_t r2_len;
    uint8_t* r2 = aes_decrypt_ecb(r1, r1_len, data->key1, &r2_len);
    safe_free(r1);
    
    if (!r2) return NULL;
    
    // 移除尾部的零字节填充
    while (r2_len > 0 && r2[r2_len - 1] == 0) {
        r2_len--;
    }
    
    // 转换为字符串
    char* result = safe_malloc(r2_len + 1);
    memcpy(result, r2, r2_len);
    result[r2_len] = '\0';
    safe_free(r2);
    
    return result;
}

// 销毁函数
static void aes_ecb_destroy(cipher_interface_t* self) {
    if (self) {
        safe_free(self->private_data);
        safe_free(self);
    }
}

// 创建AES-ECB加解密实例
cipher_interface_t* create_aes_ecb_cipher(const uint8_t* key1, const uint8_t* key2) {
    if (!key1 || !key2) return NULL;
    
    cipher_interface_t* cipher = safe_malloc(sizeof(cipher_interface_t));
    aes_ecb_data_t* data = safe_malloc(sizeof(aes_ecb_data_t));
    
    // 复制密钥
    memcpy(data->key1, key1, 16);
    memcpy(data->key2, key2, 16);
    
    // 设置函数指针
    cipher->encrypt = aes_ecb_encrypt;
    cipher->decrypt = aes_ecb_decrypt;
    cipher->destroy = aes_ecb_destroy;
    cipher->private_data = data;
    
    return cipher;
}