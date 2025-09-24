//
// Created by bad_g on 2025/9/25.
//

#ifndef ESURFINGCLIENT_MODXTEA_H
#define ESURFINGCLIENT_MODXTEA_H

#include <stdint.h>

#define MODXTEA_NUM_ROUNDS 32
#define MODXTEA_DELTA 0x9E3779B9
#define MODXTEA_BLOCK_SIZE 8

// ModXTEA上下文结构体
typedef struct {
    uint32_t key1[4];
    uint32_t key2[4];
    uint32_t key3[4];
    uint32_t iv[2];  // 仅用于ModXTEA with IV模式
    int has_iv;      // 是否使用IV
} modxtea_context_t;

// 核心函数
void modxtea_init(modxtea_context_t* ctx, const uint32_t* key1, const uint32_t* key2,
                  const uint32_t* key3, const uint32_t* iv);
void modxtea_encrypt_block(uint32_t v0, uint32_t v1, const uint32_t* key,
                          uint32_t* out_v0, uint32_t* out_v1);
void modxtea_decrypt_block(uint32_t v0, uint32_t v1, const uint32_t* key,
                          uint32_t* out_v0, uint32_t* out_v1);

// 高级加密解密函数
int modxtea_encrypt(const uint32_t* key1, const uint32_t* key2, const uint32_t* key3,
                    const uint8_t* plaintext, size_t plaintext_len,
                    uint8_t** ciphertext, size_t* ciphertext_len);
int modxtea_decrypt(const uint32_t* key1, const uint32_t* key2, const uint32_t* key3,
                    const uint8_t* ciphertext, size_t ciphertext_len,
                    uint8_t** plaintext, size_t* plaintext_len);

// ModXTEA with IV模式
int modxtea_encrypt_iv(const uint32_t* key1, const uint32_t* key2, const uint32_t* key3,
                       const uint32_t* iv, const uint8_t* plaintext, size_t plaintext_len,
                       uint8_t** ciphertext, size_t* ciphertext_len);
int modxtea_decrypt_iv(const uint32_t* key1, const uint32_t* key2, const uint32_t* key3,
                       const uint32_t* iv, const uint8_t* ciphertext, size_t ciphertext_len,
                       uint8_t** plaintext, size_t* plaintext_len);

// 工具函数
uint32_t modxtea_get_uint32_be(const uint8_t* data, size_t offset);
void modxtea_set_uint32_be(uint8_t* data, size_t offset, uint32_t value);
uint8_t* modxtea_pad_to_multiple_of_8(const uint8_t* data, size_t data_len, size_t* padded_len);
uint8_t* modxtea_remove_padding(const uint8_t* data, size_t data_len, size_t* unpadded_len);

#endif //ESURFINGCLIENT_MODXTEA_H