//
// Created by bad_g on 2025/9/25.
//
#include <stdlib.h>
#include <string.h>

#include "../../headFiles/cipher/impl/SM4.h"

// SM4 S盒
static const uint8_t SM4_Sbox[256] = {
    0xd6, 0x90, 0xe9, 0xfe, 0xcc, 0xe1, 0x3d, 0xb7, 0x16, 0xb6, 0x14, 0xc2, 0x28, 0xfb, 0x2c, 0x05,
    0x2b, 0x67, 0x9a, 0x76, 0x2a, 0xbe, 0x04, 0xc3, 0xaa, 0x44, 0x13, 0x26, 0x49, 0x86, 0x06, 0x99,
    0x9c, 0x42, 0x50, 0xf4, 0x91, 0xef, 0x98, 0x7a, 0x33, 0x54, 0x0b, 0x43, 0xed, 0xcf, 0xac, 0x62,
    0xe4, 0xb3, 0x1c, 0xa9, 0xc9, 0x08, 0xe8, 0x95, 0x80, 0xdf, 0x94, 0xfa, 0x75, 0x8f, 0x3f, 0xa6,
    0x47, 0x07, 0xa7, 0xfc, 0xf3, 0x73, 0x17, 0xba, 0x83, 0x59, 0x3c, 0x19, 0xe6, 0x85, 0x4f, 0xa8,
    0x68, 0x6b, 0x81, 0xb2, 0x71, 0x64, 0xda, 0x8b, 0xf8, 0xeb, 0x0f, 0x4b, 0x70, 0x56, 0x9d, 0x35,
    0x1e, 0x24, 0x0e, 0x5e, 0x63, 0x58, 0xd1, 0xa2, 0x25, 0x22, 0x7c, 0x3b, 0x01, 0x21, 0x78, 0x87,
    0xd4, 0x00, 0x46, 0x57, 0x9f, 0xd3, 0x27, 0x52, 0x4c, 0x36, 0x02, 0xe7, 0xa0, 0xc4, 0xc8, 0x9e,
    0xea, 0xbf, 0x8a, 0xd2, 0x40, 0xc7, 0x38, 0xb5, 0xa3, 0xf7, 0xf2, 0xce, 0xf9, 0x61, 0x15, 0xa1,
    0xe0, 0xae, 0x5d, 0xa4, 0x9b, 0x34, 0x1a, 0x55, 0xad, 0x93, 0x32, 0x30, 0xf5, 0x8c, 0xb1, 0xe3,
    0x1d, 0xf6, 0xe2, 0x2e, 0x82, 0x66, 0xca, 0x60, 0xc0, 0x29, 0x23, 0xab, 0x0d, 0x53, 0x4e, 0x6f,
    0xd5, 0xdb, 0x37, 0x45, 0xde, 0xfd, 0x8e, 0x2f, 0x03, 0xff, 0x6a, 0x72, 0x6d, 0x6c, 0x5b, 0x51,
    0x8d, 0x1b, 0xaf, 0x92, 0xbb, 0xdd, 0xbc, 0x7f, 0x11, 0xd9, 0x5c, 0x41, 0x1f, 0x10, 0x5a, 0xd8,
    0x0a, 0xc1, 0x31, 0x88, 0xa5, 0xcd, 0x7b, 0xbd, 0x2d, 0x74, 0xd0, 0x12, 0xb8, 0xe5, 0xb4, 0xb0,
    0x89, 0x69, 0x97, 0x4a, 0x0c, 0x96, 0x77, 0x7e, 0x65, 0xb9, 0xf1, 0x09, 0xc5, 0x6e, 0xc6, 0x84,
    0x18, 0xf0, 0x7d, 0xec, 0x3a, 0xdc, 0x4d, 0x20, 0x79, 0xee, 0x5f, 0x3e, 0xd7, 0xcb, 0x39, 0x48
};

// SM4常数CK
static const uint32_t SM4_CK[32] = {
    0x00070e15, 0x1c232a31, 0x383f464d, 0x545b6269,
    0x70777e85, 0x8c939aa1, 0xa8afb6bd, 0xc4cbd2d9,
    0xe0e7eef5, 0xfc030a11, 0x181f262d, 0x343b4249,
    0x50575e65, 0x6c737a81, 0x888f969d, 0xa4abb2b9,
    0xc0c7ced5, 0xdce3eaf1, 0xf8ff060d, 0x141b2229,
    0x30373e45, 0x4c535a61, 0x686f767d, 0x848b9299,
    0xa0a7aeb5, 0xbcc3cad1, 0xd8dfe6ed, 0xf4fb0209,
    0x10171e25, 0x2c333a41, 0x484f565d, 0x646b7279
};

// SM4常数FK
static const uint32_t SM4_FK[4] = {
    0xa3b1bac6, 0x56aa3350, 0x677d9197, 0xb27022dc
};

// 循环左移
static uint32_t sm4_rotate_left(uint32_t x, int bits) {
    return (x << bits) | (x >> (32 - bits));
}

// τ变换
static uint32_t sm4_tau(uint32_t A) {
    uint8_t a0 = (A >> 24) & 0xFF;
    uint8_t a1 = (A >> 16) & 0xFF;
    uint8_t a2 = (A >> 8) & 0xFF;
    uint8_t a3 = A & 0xFF;

    return (SM4_Sbox[a0] << 24) | (SM4_Sbox[a1] << 16) | (SM4_Sbox[a2] << 8) | SM4_Sbox[a3];
}

// L'变换（用于密钥扩展）
static uint32_t sm4_L_ap(uint32_t B) {
    return B ^ sm4_rotate_left(B, 13) ^ sm4_rotate_left(B, 23);
}

// T'变换（用于密钥扩展）
static uint32_t sm4_T_ap(uint32_t Z) {
    return sm4_L_ap(sm4_tau(Z));
}

// L变换（用于加密）
static uint32_t sm4_L(uint32_t B) {
    return B ^ sm4_rotate_left(B, 2) ^ sm4_rotate_left(B, 10) ^ sm4_rotate_left(B, 18) ^ sm4_rotate_left(B, 24);
}

// T变换（用于加密）
static uint32_t sm4_T(uint32_t Z) {
    return sm4_L(sm4_tau(Z));
}

// F函数
static uint32_t sm4_F(const uint32_t* X, uint32_t rk, int round) {
    switch (round % 4) {
        case 0: return X[0] ^ sm4_T(X[1] ^ X[2] ^ X[3] ^ rk);
        case 1: return X[1] ^ sm4_T(X[2] ^ X[3] ^ X[0] ^ rk);
        case 2: return X[2] ^ sm4_T(X[3] ^ X[0] ^ X[1] ^ rk);
        case 3: return X[3] ^ sm4_T(X[0] ^ X[1] ^ X[2] ^ rk);
        default: return 0;
    }
}

// 密钥扩展
static void sm4_expand_key(int for_encryption, const uint8_t* key, uint32_t* rk) {
    uint32_t K[4];
    uint32_t MK[4];

    // 将密钥转换为32位整数
    for (int i = 0; i < 4; i++) {
        MK[i] = sm4_big_endian_to_int(key, i * 4);
    }

    // 初始化K
    for (int i = 0; i < 4; i++) {
        K[i] = MK[i] ^ SM4_FK[i];
    }

    // 生成轮密钥
    for (int i = 0; i < 32; i++) {
        K[(i + 4) % 4] = K[i % 4] ^ sm4_T_ap(K[(i + 1) % 4] ^ K[(i + 2) % 4] ^ K[(i + 3) % 4] ^ SM4_CK[i]);
        if (for_encryption) {
            rk[i] = K[(i + 4) % 4];
        } else {
            rk[31 - i] = K[(i + 4) % 4];
        }
    }
}

// 大端序转换
static void sm4_int_to_big_endian(uint32_t val, uint8_t* buf, int offset) {
    buf[offset] = (val >> 24) & 0xFF;
    buf[offset + 1] = (val >> 16) & 0xFF;
    buf[offset + 2] = (val >> 8) & 0xFF;
    buf[offset + 3] = val & 0xFF;
}

static uint32_t sm4_big_endian_to_int(const uint8_t* buf, int offset) {
    return ((uint32_t)buf[offset] << 24) |
           ((uint32_t)buf[offset + 1] << 16) |
           ((uint32_t)buf[offset + 2] << 8) |
           (uint32_t)buf[offset + 3];
}

// 初始化SM4上下文
void sm4_init(sm4_context_t* ctx, int for_encryption, const uint8_t* key) {
    ctx->for_encryption = for_encryption;
    sm4_expand_key(for_encryption, key, ctx->rk);
}

// 处理单个块
void sm4_process_block(sm4_context_t* ctx, const uint8_t* input, uint8_t* output) {
    uint32_t X[4];

    // 输入转换
    for (int i = 0; i < 4; i++) {
        X[i] = sm4_big_endian_to_int(input, i * 4);
    }

    // 32轮迭代
    for (int i = 0; i < 32; i++) {
        uint32_t temp = sm4_F(X, ctx->rk[i], i);
        X[0] = X[1];
        X[1] = X[2];
        X[2] = X[3];
        X[3] = temp;
    }

    // 输出转换（反序）
    sm4_int_to_big_endian(X[3], output, 0);
    sm4_int_to_big_endian(X[2], output, 4);
    sm4_int_to_big_endian(X[1], output, 8);
    sm4_int_to_big_endian(X[0], output, 12);
}

// PKCS7填充
uint8_t* sm4_pkcs7_padding(const uint8_t* data, size_t data_len, size_t* padded_len) {
    size_t padding_len = SM4_BLOCK_SIZE - (data_len % SM4_BLOCK_SIZE);
    *padded_len = data_len + padding_len;

    uint8_t* padded = malloc(*padded_len);
    if (!padded) return NULL;

    memcpy(padded, data, data_len);
    for (size_t i = data_len; i < *padded_len; i++) {
        padded[i] = (uint8_t)padding_len;
    }

    return padded;
}

// PKCS7去填充
uint8_t* sm4_pkcs7_unpadding(const uint8_t* data, size_t data_len, size_t* unpadded_len) {
    if (data_len == 0) return NULL;

    uint8_t padding_len = data[data_len - 1];
    if (padding_len > SM4_BLOCK_SIZE || padding_len > data_len) return NULL;

    *unpadded_len = data_len - padding_len;
    uint8_t* unpadded = malloc(*unpadded_len);
    if (!unpadded) return NULL;

    memcpy(unpadded, data, *unpadded_len);
    return unpadded;
}

// ECB加密
int sm4_encrypt_ecb(const uint8_t* key, const uint8_t* plaintext, size_t plaintext_len,
                    uint8_t** ciphertext, size_t* ciphertext_len) {
    if (!key || !plaintext || !ciphertext || !ciphertext_len) return -1;

    // PKCS7填充
    size_t padded_len;
    uint8_t* padded = sm4_pkcs7_padding(plaintext, plaintext_len, &padded_len);
    if (!padded) return -1;

    *ciphertext_len = padded_len;
    *ciphertext = malloc(*ciphertext_len);
    if (!*ciphertext) {
        free(padded);
        return -1;
    }

    sm4_context_t ctx;
    sm4_init(&ctx, 1, key);

    // 分块加密
    for (size_t i = 0; i < padded_len; i += SM4_BLOCK_SIZE) {
        sm4_process_block(&ctx, padded + i, *ciphertext + i);
    }

    free(padded);
    return 0;
}

// ECB解密
int sm4_decrypt_ecb(const uint8_t* key, const uint8_t* ciphertext, size_t ciphertext_len,
                    uint8_t** plaintext, size_t* plaintext_len) {
    if (!key || !ciphertext || !plaintext || !plaintext_len) return -1;
    if (ciphertext_len % SM4_BLOCK_SIZE != 0) return -1;

    uint8_t* decrypted = malloc(ciphertext_len);
    if (!decrypted) return -1;

    sm4_context_t ctx;
    sm4_init(&ctx, 0, key);

    // 分块解密
    for (size_t i = 0; i < ciphertext_len; i += SM4_BLOCK_SIZE) {
        sm4_process_block(&ctx, ciphertext + i, decrypted + i);
    }

    // 去填充
    *plaintext = sm4_pkcs7_unpadding(decrypted, ciphertext_len, plaintext_len);
    free(decrypted);

    return *plaintext ? 0 : -1;
}

// CBC加密
int sm4_encrypt_cbc(const uint8_t* key, const uint8_t* iv, const uint8_t* plaintext, size_t plaintext_len,
                    uint8_t** ciphertext, size_t* ciphertext_len) {
    if (!key || !iv || !plaintext || !ciphertext || !ciphertext_len) return -1;

    // PKCS7填充
    size_t padded_len;
    uint8_t* padded = sm4_pkcs7_padding(plaintext, plaintext_len, &padded_len);
    if (!padded) return -1;

    *ciphertext_len = padded_len;
    *ciphertext = malloc(*ciphertext_len);
    if (!*ciphertext) {
        free(padded);
        return -1;
    }

    sm4_context_t ctx;
    sm4_init(&ctx, 1, key);

    uint8_t prev_block[SM4_BLOCK_SIZE];
    memcpy(prev_block, iv, SM4_BLOCK_SIZE);

    // 分块加密
    for (size_t i = 0; i < padded_len; i += SM4_BLOCK_SIZE) {
        uint8_t xor_block[SM4_BLOCK_SIZE];

        // XOR with previous block
        for (int j = 0; j < SM4_BLOCK_SIZE; j++) {
            xor_block[j] = padded[i + j] ^ prev_block[j];
        }

        sm4_process_block(&ctx, xor_block, *ciphertext + i);
        memcpy(prev_block, *ciphertext + i, SM4_BLOCK_SIZE);
    }

    free(padded);
    return 0;
}

// CBC解密
int sm4_decrypt_cbc(const uint8_t* key, const uint8_t* iv, const uint8_t* ciphertext, size_t ciphertext_len,
                    uint8_t** plaintext, size_t* plaintext_len) {
    if (!key || !iv || !ciphertext || !plaintext || !plaintext_len) return -1;
    if (ciphertext_len % SM4_BLOCK_SIZE != 0) return -1;

    uint8_t* decrypted = malloc(ciphertext_len);
    if (!decrypted) return -1;

    sm4_context_t ctx;
    sm4_init(&ctx, 0, key);

    uint8_t prev_block[SM4_BLOCK_SIZE];
    memcpy(prev_block, iv, SM4_BLOCK_SIZE);

    // 分块解密
    for (size_t i = 0; i < ciphertext_len; i += SM4_BLOCK_SIZE) {
        uint8_t temp_block[SM4_BLOCK_SIZE];
        memcpy(temp_block, ciphertext + i, SM4_BLOCK_SIZE);

        sm4_process_block(&ctx, ciphertext + i, decrypted + i);

        // XOR with previous block
        for (int j = 0; j < SM4_BLOCK_SIZE; j++) {
            decrypted[i + j] ^= prev_block[j];
        }

        memcpy(prev_block, temp_block, SM4_BLOCK_SIZE);
    }

    // 去填充
    *plaintext = sm4_pkcs7_unpadding(decrypted, ciphertext_len, plaintext_len);
    free(decrypted);

    return *plaintext ? 0 : -1;
}