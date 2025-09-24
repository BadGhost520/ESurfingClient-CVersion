//
// Created by bad_g on 2025/9/25.
//

#ifndef ESURFINGCLIENT_DES_H
#define ESURFINGCLIENT_DES_H

#include <stdint.h>

#define DES_BLOCK_SIZE 8
#define DES_KEY_SIZE 8
#define TDES_KEY_SIZE 24

// DES上下文结构体
typedef struct {
    uint64_t subkeys[16];  // 16个子密钥
    int encrypt_mode;      // 1为加密，0为解密
} des_context_t;

// 3DES上下文结构体
typedef struct {
    des_context_t des1;
    des_context_t des2;
    des_context_t des3;
} tdes_context_t;

// 核心DES函数
void des_init(des_context_t* ctx, int encrypt_mode, const uint8_t* key);
void des_process_block(des_context_t* ctx, const uint8_t* input, uint8_t* output);

// 3DES函数
void tdes_init(tdes_context_t* ctx, int encrypt_mode, const uint8_t* key);
void tdes_process_block(tdes_context_t* ctx, const uint8_t* input, uint8_t* output);

// ECB模式
int tdes_encrypt_ecb(const uint8_t* key, const uint8_t* plaintext, size_t plaintext_len,
                     uint8_t** ciphertext, size_t* ciphertext_len);
int tdes_decrypt_ecb(const uint8_t* key, const uint8_t* ciphertext, size_t ciphertext_len,
                     uint8_t** plaintext, size_t* plaintext_len);

// CBC模式
int tdes_encrypt_cbc(const uint8_t* key, const uint8_t* iv, const uint8_t* plaintext, size_t plaintext_len,
                     uint8_t** ciphertext, size_t* ciphertext_len);
int tdes_decrypt_cbc(const uint8_t* key, const uint8_t* iv, const uint8_t* ciphertext, size_t ciphertext_len,
                     uint8_t** plaintext, size_t* plaintext_len);

// 双重3DES加密（模拟Kotlin中的两次加密）
int tdes_double_encrypt_ecb(const uint8_t* key1, const uint8_t* key2, const uint8_t* plaintext, size_t plaintext_len,
                           uint8_t** ciphertext, size_t* ciphertext_len);
int tdes_double_decrypt_ecb(const uint8_t* key1, const uint8_t* key2, const uint8_t* ciphertext, size_t ciphertext_len,
                           uint8_t** plaintext, size_t* plaintext_len);

int tdes_double_encrypt_cbc(const uint8_t* key1, const uint8_t* key2, const uint8_t* iv,
                           const uint8_t* plaintext, size_t plaintext_len,
                           uint8_t** ciphertext, size_t* ciphertext_len);
int tdes_double_decrypt_cbc(const uint8_t* key1, const uint8_t* key2, const uint8_t* iv,
                           const uint8_t* ciphertext, size_t ciphertext_len,
                           uint8_t** plaintext, size_t* plaintext_len);

// 内部辅助函数
void des_key_schedule(const uint8_t* key, uint64_t* subkeys, int encrypt_mode);
uint64_t des_initial_permutation(uint64_t input);
uint64_t des_final_permutation(uint64_t input);
uint64_t des_feistel_function(uint32_t right, uint64_t subkey);
uint32_t des_s_box_substitution(uint64_t input);
uint32_t des_permutation(uint32_t input);

// 工具函数
uint64_t des_bytes_to_uint64(const uint8_t* bytes);
void des_uint64_to_bytes(uint64_t value, uint8_t* bytes);
uint8_t* des_pad_to_multiple_of_8(const uint8_t* data, size_t data_len, size_t* padded_len);
uint8_t* des_remove_padding(const uint8_t* data, size_t data_len, size_t* unpadded_len);

#endif //ESURFINGCLIENT_DES_H