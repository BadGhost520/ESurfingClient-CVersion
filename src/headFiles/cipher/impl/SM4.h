//
// Created by bad_g on 2025/9/25.
//

#ifndef ESURFINGCLIENT_SM4_H
#define ESURFINGCLIENT_SM4_H

#include <stdint.h>
#include <stddef.h>

#define SM4_BLOCK_SIZE 16
#define SM4_KEY_SIZE 16

// SM4上下文结构体
typedef struct {
    uint32_t rk[32];  // 轮密钥
    int for_encryption;  // 加密标志
} sm4_context_t;

// SM4函数声明
void sm4_init(sm4_context_t* ctx, int for_encryption, const uint8_t* key);
void sm4_process_block(sm4_context_t* ctx, const uint8_t* input, uint8_t* output);

// ECB模式函数
int sm4_encrypt_ecb(const uint8_t* key, const uint8_t* plaintext, size_t plaintext_len,
                    uint8_t** ciphertext, size_t* ciphertext_len);
int sm4_decrypt_ecb(const uint8_t* key, const uint8_t* ciphertext, size_t ciphertext_len,
                    uint8_t** plaintext, size_t* plaintext_len);

// CBC模式函数
int sm4_encrypt_cbc(const uint8_t* key, const uint8_t* iv, const uint8_t* plaintext, size_t plaintext_len,
                    uint8_t** ciphertext, size_t* ciphertext_len);
int sm4_decrypt_cbc(const uint8_t* key, const uint8_t* iv, const uint8_t* ciphertext, size_t ciphertext_len,
                    uint8_t** plaintext, size_t* plaintext_len);

// 内部辅助函数
static uint32_t sm4_rotate_left(uint32_t x, int bits);
static uint32_t sm4_tau(uint32_t A);
static uint32_t sm4_L_ap(uint32_t B);
static uint32_t sm4_T_ap(uint32_t Z);
static uint32_t sm4_L(uint32_t B);
static uint32_t sm4_T(uint32_t Z);
static uint32_t sm4_F(const uint32_t* X, uint32_t rk, int round);
static void sm4_expand_key(int for_encryption, const uint8_t* key, uint32_t* rk);

// 工具函数
static void sm4_int_to_big_endian(uint32_t val, uint8_t* buf, int offset);
static uint32_t sm4_big_endian_to_int(const uint8_t* buf, int offset);

// PKCS7填充函数
uint8_t* sm4_pkcs7_padding(const uint8_t* data, size_t data_len, size_t* padded_len);
uint8_t* sm4_pkcs7_unpadding(const uint8_t* data, size_t data_len, size_t* unpadded_len);

#endif //ESURFINGCLIENT_SM4_H