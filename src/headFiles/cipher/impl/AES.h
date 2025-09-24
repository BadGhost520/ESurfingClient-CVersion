//
// Created by bad_g on 2025/9/25.
//
#ifndef ESURFINGCLIENT_AES_H
#define ESURFINGCLIENT_AES_H

#include <stdint.h>

#define AES_BLOCK_SIZE 16
#define AES_KEY_SIZE 16
#define AES_ROUNDS 10

// AES上下文结构体
typedef struct {
    uint32_t round_keys[44];  // 11轮密钥，每轮4个32位字
    int encrypt_mode;         // 1为加密，0为解密
} aes_context_t;

// 核心函数
void aes_init(aes_context_t* ctx, int encrypt_mode, const uint8_t* key);
void aes_process_block(aes_context_t* ctx, const uint8_t* input, uint8_t* output);

// ECB模式
int aes_encrypt_ecb(const uint8_t* key, const uint8_t* plaintext, size_t plaintext_len,
                    uint8_t** ciphertext, size_t* ciphertext_len);
int aes_decrypt_ecb(const uint8_t* key, const uint8_t* ciphertext, size_t ciphertext_len,
                    uint8_t** plaintext, size_t* plaintext_len);

// CBC模式
int aes_encrypt_cbc(const uint8_t* key, const uint8_t* iv, const uint8_t* plaintext, size_t plaintext_len,
                    uint8_t** ciphertext, size_t* ciphertext_len);
int aes_decrypt_cbc(const uint8_t* key, const uint8_t* iv, const uint8_t* ciphertext, size_t ciphertext_len,
                    uint8_t** plaintext, size_t* plaintext_len);

// 双重AES加密（模拟Kotlin中的两次加密）
int aes_double_encrypt_ecb(const uint8_t* key1, const uint8_t* key2, const uint8_t* plaintext, size_t plaintext_len,
                          uint8_t** ciphertext, size_t* ciphertext_len);
int aes_double_decrypt_ecb(const uint8_t* key1, const uint8_t* key2, const uint8_t* ciphertext, size_t ciphertext_len,
                          uint8_t** plaintext, size_t* plaintext_len);

int aes_double_encrypt_cbc(const uint8_t* key1, const uint8_t* key2, const uint8_t* iv,
                          const uint8_t* plaintext, size_t plaintext_len,
                          uint8_t** ciphertext, size_t* ciphertext_len);
int aes_double_decrypt_cbc(const uint8_t* key1, const uint8_t* key2, const uint8_t* iv,
                          const uint8_t* ciphertext, size_t ciphertext_len,
                          uint8_t** plaintext, size_t* plaintext_len);

// 内部辅助函数
void aes_key_expansion(const uint8_t* key, uint32_t* round_keys);
void aes_add_round_key(uint8_t* state, const uint32_t* round_key);
void aes_sub_bytes(uint8_t* state);
void aes_inv_sub_bytes(uint8_t* state);
void aes_shift_rows(uint8_t* state);
void aes_inv_shift_rows(uint8_t* state);
void aes_mix_columns(uint8_t* state);
void aes_inv_mix_columns(uint8_t* state);

// 工具函数
uint8_t* aes_pad_pkcs7(const uint8_t* data, size_t data_len, size_t* padded_len);
uint8_t* aes_unpad_pkcs7(const uint8_t* data, size_t data_len, size_t* unpadded_len);

#endif //ESURFINGCLIENT_AES_H