//
// Created by bad_g on 2025/9/25.
//
#include <stdlib.h>
#include <string.h>

#include "../../headFiles/cipher/impl/DES.h"

// DES初始置换表
static const int DES_IP[64] = {
    58, 50, 42, 34, 26, 18, 10, 2,
    60, 52, 44, 36, 28, 20, 12, 4,
    62, 54, 46, 38, 30, 22, 14, 6,
    64, 56, 48, 40, 32, 24, 16, 8,
    57, 49, 41, 33, 25, 17, 9, 1,
    59, 51, 43, 35, 27, 19, 11, 3,
    61, 53, 45, 37, 29, 21, 13, 5,
    63, 55, 47, 39, 31, 23, 15, 7
};

// DES最终置换表
static const int DES_FP[64] = {
    40, 8, 48, 16, 56, 24, 64, 32,
    39, 7, 47, 15, 55, 23, 63, 31,
    38, 6, 46, 14, 54, 22, 62, 30,
    37, 5, 45, 13, 53, 21, 61, 29,
    36, 4, 44, 12, 52, 20, 60, 28,
    35, 3, 43, 11, 51, 19, 59, 27,
    34, 2, 42, 10, 50, 18, 58, 26,
    33, 1, 41, 9, 49, 17, 57, 25
};

// DES S盒（简化版本）
static const int DES_SBOX[8][64] = {
    // S1
    {14, 4, 13, 1, 2, 15, 11, 8, 3, 10, 6, 12, 5, 9, 0, 7,
     0, 15, 7, 4, 14, 2, 13, 1, 10, 6, 12, 11, 9, 5, 3, 8,
     4, 1, 14, 8, 13, 6, 2, 11, 15, 12, 9, 7, 3, 10, 5, 0,
     15, 12, 8, 2, 4, 9, 1, 7, 5, 11, 3, 14, 10, 0, 6, 13},
    // S2-S8 (简化，实际应用中需要完整的S盒)
    {15, 1, 8, 14, 6, 11, 3, 4, 9, 7, 2, 13, 12, 0, 5, 10,
     3, 13, 4, 7, 15, 2, 8, 14, 12, 0, 1, 10, 6, 9, 11, 5,
     0, 14, 7, 11, 10, 4, 13, 1, 5, 8, 12, 6, 9, 3, 2, 15,
     13, 8, 10, 1, 3, 15, 4, 2, 11, 6, 7, 12, 0, 5, 14, 9},
    {10, 0, 9, 14, 6, 3, 15, 5, 1, 13, 12, 7, 11, 4, 2, 8,
     13, 7, 0, 9, 3, 4, 6, 10, 2, 8, 5, 14, 12, 11, 15, 1,
     13, 6, 4, 9, 8, 15, 3, 0, 11, 1, 2, 12, 5, 10, 14, 7,
     1, 10, 13, 0, 6, 9, 8, 7, 4, 15, 14, 3, 11, 5, 2, 12},
    {7, 13, 14, 3, 0, 6, 9, 10, 1, 2, 8, 5, 11, 12, 4, 15,
     13, 8, 11, 5, 6, 15, 0, 3, 4, 7, 2, 12, 1, 10, 14, 9,
     10, 6, 9, 0, 12, 11, 7, 13, 15, 1, 3, 14, 5, 2, 8, 4,
     3, 15, 0, 6, 10, 1, 13, 8, 9, 4, 5, 11, 12, 7, 2, 14},
    {2, 12, 4, 1, 7, 10, 11, 6, 8, 5, 3, 15, 13, 0, 14, 9,
     14, 11, 2, 12, 4, 7, 13, 1, 5, 0, 15, 10, 3, 9, 8, 6,
     4, 2, 1, 11, 10, 13, 7, 8, 15, 9, 12, 5, 6, 3, 0, 14,
     11, 8, 12, 7, 1, 14, 2, 13, 6, 15, 0, 9, 10, 4, 5, 3},
    {12, 1, 10, 15, 9, 2, 6, 8, 0, 13, 3, 4, 14, 7, 5, 11,
     10, 15, 4, 2, 7, 12, 9, 5, 6, 1, 13, 14, 0, 11, 3, 8,
     9, 14, 15, 5, 2, 8, 12, 3, 7, 0, 4, 10, 1, 13, 11, 6,
     4, 3, 2, 12, 9, 5, 15, 10, 11, 14, 1, 7, 6, 0, 8, 13},
    {4, 11, 2, 14, 15, 0, 8, 13, 3, 12, 9, 7, 5, 10, 6, 1,
     13, 0, 11, 7, 4, 9, 1, 10, 14, 3, 5, 12, 2, 15, 8, 6,
     1, 4, 11, 13, 12, 3, 7, 14, 10, 15, 6, 8, 0, 5, 9, 2,
     6, 11, 13, 8, 1, 4, 10, 7, 9, 5, 0, 15, 14, 2, 3, 12},
    {13, 2, 8, 4, 6, 15, 11, 1, 10, 9, 3, 14, 5, 0, 12, 7,
     1, 15, 13, 8, 10, 3, 7, 4, 12, 5, 6, 11, 0, 14, 9, 2,
     7, 11, 4, 1, 9, 12, 14, 2, 0, 6, 10, 13, 15, 3, 5, 8,
     2, 1, 14, 7, 4, 10, 8, 13, 15, 12, 9, 0, 3, 5, 6, 11}
};

// 字节数组转64位整数
uint64_t des_bytes_to_uint64(const uint8_t* bytes) {
    uint64_t result = 0;
    for (int i = 0; i < 8; i++) {
        result = (result << 8) | bytes[i];
    }
    return result;
}

// 64位整数转字节数组
void des_uint64_to_bytes(uint64_t value, uint8_t* bytes) {
    for (int i = 7; i >= 0; i--) {
        bytes[i] = value & 0xFF;
        value >>= 8;
    }
}

// 填充到8字节的倍数
uint8_t* des_pad_to_multiple_of_8(const uint8_t* data, size_t data_len, size_t* padded_len) {
    size_t padding = (8 - (data_len % 8)) % 8;
    if (padding == 0) padding = 8;

    *padded_len = data_len + padding;
    uint8_t* padded = malloc(*padded_len);
    if (!padded) return NULL;

    memcpy(padded, data, data_len);
    for (size_t i = data_len; i < *padded_len; i++) {
        padded[i] = 0;  // 简化填充，使用零填充
    }

    return padded;
}

// 去除填充
uint8_t* des_remove_padding(const uint8_t* data, size_t data_len, size_t* unpadded_len) {
    *unpadded_len = data_len;
    while (*unpadded_len > 0 && data[*unpadded_len - 1] == 0) {
        (*unpadded_len)--;
    }

    uint8_t* unpadded = malloc(*unpadded_len);
    if (!unpadded) return NULL;

    memcpy(unpadded, data, *unpadded_len);
    return unpadded;
}

// 简化的DES密钥调度
void des_key_schedule(const uint8_t* key, uint64_t* subkeys, int encrypt_mode) {
    // 简化实现：直接使用密钥的变换作为子密钥
    uint64_t key64 = des_bytes_to_uint64(key);

    for (int i = 0; i < 16; i++) {
        if (encrypt_mode) {
            subkeys[i] = key64 ^ (0x0123456789ABCDEFULL >> (i * 4));
        } else {
            subkeys[15 - i] = key64 ^ (0x0123456789ABCDEFULL >> (i * 4));
        }
    }
}

// 简化的DES初始置换
uint64_t des_initial_permutation(uint64_t input) {
    // 简化实现
    return ((input & 0xF0F0F0F0F0F0F0F0ULL) >> 4) | ((input & 0x0F0F0F0F0F0F0F0FULL) << 4);
}

// 简化的DES最终置换
uint64_t des_final_permutation(uint64_t input) {
    // 简化实现
    return ((input & 0xF0F0F0F0F0F0F0F0ULL) >> 4) | ((input & 0x0F0F0F0F0F0F0F0FULL) << 4);
}

// 简化的DES F函数
uint64_t des_feistel_function(uint32_t right, uint64_t subkey) {
    // 简化实现
    uint64_t expanded = ((uint64_t)right << 16) | right;
    uint64_t xored = expanded ^ subkey;

    // 简化的S盒替换
    uint32_t result = 0;
    for (int i = 0; i < 8; i++) {
        int sbox_input = (xored >> (i * 6)) & 0x3F;
        result |= (DES_SBOX[i][sbox_input] << (i * 4));
    }

    return result;
}

// DES初始化
void des_init(des_context_t* ctx, int encrypt_mode, const uint8_t* key) {
    ctx->encrypt_mode = encrypt_mode;
    des_key_schedule(key, ctx->subkeys, encrypt_mode);
}

// DES块处理
void des_process_block(des_context_t* ctx, const uint8_t* input, uint8_t* output) {
    uint64_t block = des_bytes_to_uint64(input);

    // 初始置换
    block = des_initial_permutation(block);

    uint32_t left = (block >> 32) & 0xFFFFFFFF;
    uint32_t right = block & 0xFFFFFFFF;

    // 16轮Feistel网络
    for (int round = 0; round < 16; round++) {
        uint32_t temp = right;
        right = left ^ (uint32_t)des_feistel_function(right, ctx->subkeys[round]);
        left = temp;
    }

    // 合并左右部分
    block = ((uint64_t)right << 32) | left;

    // 最终置换
    block = des_final_permutation(block);

    des_uint64_to_bytes(block, output);
}

// 3DES初始化
void tdes_init(tdes_context_t* ctx, int encrypt_mode, const uint8_t* key) {
    if (encrypt_mode) {
        des_init(&ctx->des1, 1, key);           // 加密
        des_init(&ctx->des2, 0, key + 8);       // 解密
        des_init(&ctx->des3, 1, key + 16);      // 加密
    } else {
        des_init(&ctx->des1, 0, key + 16);      // 解密
        des_init(&ctx->des2, 1, key + 8);       // 加密
        des_init(&ctx->des3, 0, key);           // 解密
    }
}

// 3DES块处理
void tdes_process_block(tdes_context_t* ctx, const uint8_t* input, uint8_t* output) {
    uint8_t temp1[8], temp2[8];

    des_process_block(&ctx->des1, input, temp1);
    des_process_block(&ctx->des2, temp1, temp2);
    des_process_block(&ctx->des3, temp2, output);
}

// 3DES ECB加密
int tdes_encrypt_ecb(const uint8_t* key, const uint8_t* plaintext, size_t plaintext_len,
                     uint8_t** ciphertext, size_t* ciphertext_len) {
    if (!key || !plaintext || !ciphertext || !ciphertext_len) return -1;

    // 填充
    uint8_t* padded = des_pad_to_multiple_of_8(plaintext, plaintext_len, ciphertext_len);
    if (!padded) return -1;

    *ciphertext = malloc(*ciphertext_len);
    if (!*ciphertext) {
        free(padded);
        return -1;
    }

    tdes_context_t ctx;
    tdes_init(&ctx, 1, key);

    // 分块加密
    for (size_t i = 0; i < *ciphertext_len; i += DES_BLOCK_SIZE) {
        tdes_process_block(&ctx, padded + i, *ciphertext + i);
    }

    free(padded);
    return 0;
}

// 3DES ECB解密
int tdes_decrypt_ecb(const uint8_t* key, const uint8_t* ciphertext, size_t ciphertext_len,
                     uint8_t** plaintext, size_t* plaintext_len) {
    if (!key || !ciphertext || !plaintext || !plaintext_len) return -1;

    uint8_t* decrypted = malloc(ciphertext_len);
    if (!decrypted) return -1;

    tdes_context_t ctx;
    tdes_init(&ctx, 0, key);

    // 分块解密
    for (size_t i = 0; i < ciphertext_len; i += DES_BLOCK_SIZE) {
        tdes_process_block(&ctx, ciphertext + i, decrypted + i);
    }

    // 去填充
    *plaintext = des_remove_padding(decrypted, ciphertext_len, plaintext_len);
    free(decrypted);

    return (*plaintext != NULL) ? 0 : -1;
}

// 3DES CBC加密
int tdes_encrypt_cbc(const uint8_t* key, const uint8_t* iv, const uint8_t* plaintext, size_t plaintext_len,
                     uint8_t** ciphertext, size_t* ciphertext_len) {
    if (!key || !iv || !plaintext || !ciphertext || !ciphertext_len) return -1;

    // 填充
    uint8_t* padded = des_pad_to_multiple_of_8(plaintext, plaintext_len, ciphertext_len);
    if (!padded) return -1;

    *ciphertext = malloc(*ciphertext_len);
    if (!*ciphertext) {
        free(padded);
        return -1;
    }

    tdes_context_t ctx;
    tdes_init(&ctx, 1, key);

    uint8_t prev_block[DES_BLOCK_SIZE];
    memcpy(prev_block, iv, DES_BLOCK_SIZE);

    // 分块加密
    for (size_t i = 0; i < *ciphertext_len; i += DES_BLOCK_SIZE) {
        uint8_t xor_block[DES_BLOCK_SIZE];

        // XOR with previous block
        for (int j = 0; j < DES_BLOCK_SIZE; j++) {
            xor_block[j] = padded[i + j] ^ prev_block[j];
        }

        tdes_process_block(&ctx, xor_block, *ciphertext + i);
        memcpy(prev_block, *ciphertext + i, DES_BLOCK_SIZE);
    }

    free(padded);
    return 0;
}

// 3DES CBC解密
int tdes_decrypt_cbc(const uint8_t* key, const uint8_t* iv, const uint8_t* ciphertext, size_t ciphertext_len,
                     uint8_t** plaintext, size_t* plaintext_len) {
    if (!key || !iv || !ciphertext || !plaintext || !plaintext_len) return -1;

    uint8_t* decrypted = malloc(ciphertext_len);
    if (!decrypted) return -1;

    tdes_context_t ctx;
    tdes_init(&ctx, 0, key);

    uint8_t prev_block[DES_BLOCK_SIZE];
    memcpy(prev_block, iv, DES_BLOCK_SIZE);

    // 分块解密
    for (size_t i = 0; i < ciphertext_len; i += DES_BLOCK_SIZE) {
        uint8_t temp_block[DES_BLOCK_SIZE];
        memcpy(temp_block, ciphertext + i, DES_BLOCK_SIZE);

        tdes_process_block(&ctx, ciphertext + i, decrypted + i);

        // XOR with previous block
        for (int j = 0; j < DES_BLOCK_SIZE; j++) {
            decrypted[i + j] ^= prev_block[j];
        }

        memcpy(prev_block, temp_block, DES_BLOCK_SIZE);
    }

    // 去填充
    *plaintext = des_remove_padding(decrypted, ciphertext_len, plaintext_len);
    free(decrypted);

    return (*plaintext != NULL) ? 0 : -1;
}

// 双重3DES ECB加密
int tdes_double_encrypt_ecb(const uint8_t* key1, const uint8_t* key2, const uint8_t* plaintext, size_t plaintext_len,
                           uint8_t** ciphertext, size_t* ciphertext_len) {
    uint8_t* temp_ciphertext;
    size_t temp_len;

    // 第一次加密
    int result = tdes_encrypt_ecb(key1, plaintext, plaintext_len, &temp_ciphertext, &temp_len);
    if (result != 0) return result;

    // 第二次加密
    result = tdes_encrypt_ecb(key2, temp_ciphertext, temp_len, ciphertext, ciphertext_len);
    free(temp_ciphertext);

    return result;
}

// 双重3DES ECB解密
int tdes_double_decrypt_ecb(const uint8_t* key1, const uint8_t* key2, const uint8_t* ciphertext, size_t ciphertext_len,
                           uint8_t** plaintext, size_t* plaintext_len) {
    uint8_t* temp_plaintext;
    size_t temp_len;

    // 第一次解密（使用key2）
    int result = tdes_decrypt_ecb(key2, ciphertext, ciphertext_len, &temp_plaintext, &temp_len);
    if (result != 0) return result;

    // 第二次解密（使用key1）
    result = tdes_decrypt_ecb(key1, temp_plaintext, temp_len, plaintext, plaintext_len);
    free(temp_plaintext);

    return result;
}

// 双重3DES CBC加密
int tdes_double_encrypt_cbc(const uint8_t* key1, const uint8_t* key2, const uint8_t* iv,
                           const uint8_t* plaintext, size_t plaintext_len,
                           uint8_t** ciphertext, size_t* ciphertext_len) {
    uint8_t* temp_ciphertext;
    size_t temp_len;

    // 第一次加密
    int result = tdes_encrypt_cbc(key1, iv, plaintext, plaintext_len, &temp_ciphertext, &temp_len);
    if (result != 0) return result;

    // 第二次加密
    result = tdes_encrypt_cbc(key2, iv, temp_ciphertext, temp_len, ciphertext, ciphertext_len);
    free(temp_ciphertext);

    return result;
}

// 双重3DES CBC解密
int tdes_double_decrypt_cbc(const uint8_t* key1, const uint8_t* key2, const uint8_t* iv,
                           const uint8_t* ciphertext, size_t ciphertext_len,
                           uint8_t** plaintext, size_t* plaintext_len) {
    uint8_t* temp_plaintext;
    size_t temp_len;

    // 第一次解密（使用key2）
    int result = tdes_decrypt_cbc(key2, iv, ciphertext, ciphertext_len, &temp_plaintext, &temp_len);
    if (result != 0) return result;

    // 第二次解密（使用key1）
    result = tdes_decrypt_cbc(key1, iv, temp_plaintext, temp_len, plaintext, plaintext_len);
    free(temp_plaintext);

    return result;
}