//
// Created by bad_g on 2025/9/25.
//
#include <stdlib.h>
#include <string.h>

#include "../../headFiles/cipher/impl/AES.h"

// AES S盒
static const uint8_t AES_SBOX[256] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
};

// AES 逆S盒
static const uint8_t AES_INV_SBOX[256] = {
    0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
    0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,
    0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
    0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
    0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,
    0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
    0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,
    0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,
    0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
    0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,
    0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,
    0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
    0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
    0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,
    0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
    0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d
};

// Rcon常数
static const uint8_t AES_RCON[11] = {
    0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36
};

// 有限域乘法
static uint8_t aes_gf_mul(uint8_t a, uint8_t b) {
    uint8_t result = 0;
    for (int i = 0; i < 8; i++) {
        if (b & 1) result ^= a;
        uint8_t hi_bit_set = a & 0x80;
        a <<= 1;
        if (hi_bit_set) a ^= 0x1b;
        b >>= 1;
    }
    return result;
}

// 密钥扩展
void aes_key_expansion(const uint8_t* key, uint32_t* round_keys) {
    // 复制初始密钥
    for (int i = 0; i < 4; i++) {
        round_keys[i] = ((uint32_t)key[4*i] << 24) | ((uint32_t)key[4*i+1] << 16) |
                       ((uint32_t)key[4*i+2] << 8) | (uint32_t)key[4*i+3];
    }

    // 生成轮密钥
    for (int i = 4; i < 44; i++) {
        uint32_t temp = round_keys[i-1];

        if (i % 4 == 0) {
            // RotWord
            temp = (temp << 8) | (temp >> 24);

            // SubWord
            temp = (AES_SBOX[(temp >> 24) & 0xFF] << 24) |
                   (AES_SBOX[(temp >> 16) & 0xFF] << 16) |
                   (AES_SBOX[(temp >> 8) & 0xFF] << 8) |
                   AES_SBOX[temp & 0xFF];

            // XOR with Rcon
            temp ^= (uint32_t)AES_RCON[i/4] << 24;
        }

        round_keys[i] = round_keys[i-4] ^ temp;
    }
}

// AddRoundKey
void aes_add_round_key(uint8_t* state, const uint32_t* round_key) {
    for (int i = 0; i < 4; i++) {
        uint32_t key_word = round_key[i];
        state[4*i] ^= (key_word >> 24) & 0xFF;
        state[4*i+1] ^= (key_word >> 16) & 0xFF;
        state[4*i+2] ^= (key_word >> 8) & 0xFF;
        state[4*i+3] ^= key_word & 0xFF;
    }
}

// SubBytes
void aes_sub_bytes(uint8_t* state) {
    for (int i = 0; i < 16; i++) {
        state[i] = AES_SBOX[state[i]];
    }
}

// InvSubBytes
void aes_inv_sub_bytes(uint8_t* state) {
    for (int i = 0; i < 16; i++) {
        state[i] = AES_INV_SBOX[state[i]];
    }
}

// ShiftRows
void aes_shift_rows(uint8_t* state) {
    uint8_t temp;

    // 第二行左移1位
    temp = state[1];
    state[1] = state[5];
    state[5] = state[9];
    state[9] = state[13];
    state[13] = temp;

    // 第三行左移2位
    temp = state[2];
    state[2] = state[10];
    state[10] = temp;
    temp = state[6];
    state[6] = state[14];
    state[14] = temp;

    // 第四行左移3位
    temp = state[3];
    state[3] = state[15];
    state[15] = state[11];
    state[11] = state[7];
    state[7] = temp;
}

// InvShiftRows
void aes_inv_shift_rows(uint8_t* state) {
    uint8_t temp;

    // 第二行右移1位
    temp = state[13];
    state[13] = state[9];
    state[9] = state[5];
    state[5] = state[1];
    state[1] = temp;

    // 第三行右移2位
    temp = state[2];
    state[2] = state[10];
    state[10] = temp;
    temp = state[6];
    state[6] = state[14];
    state[14] = temp;

    // 第四行右移3位
    temp = state[7];
    state[7] = state[11];
    state[11] = state[15];
    state[15] = state[3];
    state[3] = temp;
}

// MixColumns
void aes_mix_columns(uint8_t* state) {
    for (int i = 0; i < 4; i++) {
        uint8_t s0 = state[4*i];
        uint8_t s1 = state[4*i+1];
        uint8_t s2 = state[4*i+2];
        uint8_t s3 = state[4*i+3];

        state[4*i] = aes_gf_mul(0x02, s0) ^ aes_gf_mul(0x03, s1) ^ s2 ^ s3;
        state[4*i+1] = s0 ^ aes_gf_mul(0x02, s1) ^ aes_gf_mul(0x03, s2) ^ s3;
        state[4*i+2] = s0 ^ s1 ^ aes_gf_mul(0x02, s2) ^ aes_gf_mul(0x03, s3);
        state[4*i+3] = aes_gf_mul(0x03, s0) ^ s1 ^ s2 ^ aes_gf_mul(0x02, s3);
    }
}

// InvMixColumns
void aes_inv_mix_columns(uint8_t* state) {
    for (int i = 0; i < 4; i++) {
        uint8_t s0 = state[4*i];
        uint8_t s1 = state[4*i+1];
        uint8_t s2 = state[4*i+2];
        uint8_t s3 = state[4*i+3];

        state[4*i] = aes_gf_mul(0x0e, s0) ^ aes_gf_mul(0x0b, s1) ^ aes_gf_mul(0x0d, s2) ^ aes_gf_mul(0x09, s3);
        state[4*i+1] = aes_gf_mul(0x09, s0) ^ aes_gf_mul(0x0e, s1) ^ aes_gf_mul(0x0b, s2) ^ aes_gf_mul(0x0d, s3);
        state[4*i+2] = aes_gf_mul(0x0d, s0) ^ aes_gf_mul(0x09, s1) ^ aes_gf_mul(0x0e, s2) ^ aes_gf_mul(0x0b, s3);
        state[4*i+3] = aes_gf_mul(0x0b, s0) ^ aes_gf_mul(0x0d, s1) ^ aes_gf_mul(0x09, s2) ^ aes_gf_mul(0x0e, s3);
    }
}

// PKCS7填充
uint8_t* aes_pad_pkcs7(const uint8_t* data, size_t data_len, size_t* padded_len) {
    size_t padding = AES_BLOCK_SIZE - (data_len % AES_BLOCK_SIZE);
    if (padding == 0) padding = AES_BLOCK_SIZE;

    *padded_len = data_len + padding;
    uint8_t* padded = malloc(*padded_len);
    if (!padded) return NULL;

    memcpy(padded, data, data_len);
    for (size_t i = data_len; i < *padded_len; i++) {
        padded[i] = (uint8_t)padding;
    }

    return padded;
}

// PKCS7去填充
uint8_t* aes_unpad_pkcs7(const uint8_t* data, size_t data_len, size_t* unpadded_len) {
    if (data_len == 0) return NULL;

    uint8_t padding = data[data_len - 1];
    if (padding == 0 || padding > AES_BLOCK_SIZE) {
        // 无效填充，去除尾部零字节
        *unpadded_len = data_len;
        while (*unpadded_len > 0 && data[*unpadded_len - 1] == 0) {
            (*unpadded_len)--;
        }
    } else {
        *unpadded_len = data_len - padding;
    }

    uint8_t* unpadded = malloc(*unpadded_len);
    if (!unpadded) return NULL;

    memcpy(unpadded, data, *unpadded_len);
    return unpadded;
}

// 初始化AES上下文
void aes_init(aes_context_t* ctx, int encrypt_mode, const uint8_t* key) {
    ctx->encrypt_mode = encrypt_mode;
    aes_key_expansion(key, ctx->round_keys);
}

// 处理单个块
void aes_process_block(aes_context_t* ctx, const uint8_t* input, uint8_t* output) {
    uint8_t state[16];
    memcpy(state, input, 16);

    if (ctx->encrypt_mode) {
        // 加密
        aes_add_round_key(state, &ctx->round_keys[0]);

        for (int round = 1; round < AES_ROUNDS; round++) {
            aes_sub_bytes(state);
            aes_shift_rows(state);
            aes_mix_columns(state);
            aes_add_round_key(state, &ctx->round_keys[4 * round]);
        }

        aes_sub_bytes(state);
        aes_shift_rows(state);
        aes_add_round_key(state, &ctx->round_keys[40]);
    } else {
        // 解密
        aes_add_round_key(state, &ctx->round_keys[40]);

        for (int round = AES_ROUNDS - 1; round > 0; round--) {
            aes_inv_shift_rows(state);
            aes_inv_sub_bytes(state);
            aes_add_round_key(state, &ctx->round_keys[4 * round]);
            aes_inv_mix_columns(state);
        }

        aes_inv_shift_rows(state);
        aes_inv_sub_bytes(state);
        aes_add_round_key(state, &ctx->round_keys[0]);
    }

    memcpy(output, state, 16);
}

// AES ECB加密
int aes_encrypt_ecb(const uint8_t* key, const uint8_t* plaintext, size_t plaintext_len,
                    uint8_t** ciphertext, size_t* ciphertext_len) {
    if (!key || !plaintext || !ciphertext || !ciphertext_len) return -1;

    // 填充
    size_t padded_len;
    uint8_t* padded = aes_pad_pkcs7(plaintext, plaintext_len, &padded_len);
    if (!padded) return -1;

    *ciphertext_len = padded_len;
    *ciphertext = malloc(*ciphertext_len);
    if (!*ciphertext) {
        free(padded);
        return -1;
    }

    aes_context_t ctx;
    aes_init(&ctx, 1, key);

    // 分块加密
    for (size_t i = 0; i < padded_len; i += AES_BLOCK_SIZE) {
        aes_process_block(&ctx, padded + i, *ciphertext + i);
    }

    free(padded);
    return 0;
}

// AES ECB解密
int aes_decrypt_ecb(const uint8_t* key, const uint8_t* ciphertext, size_t ciphertext_len,
                    uint8_t** plaintext, size_t* plaintext_len) {
    if (!key || !ciphertext || !plaintext || !plaintext_len) return -1;

    uint8_t* decrypted = malloc(ciphertext_len);
    if (!decrypted) return -1;

    aes_context_t ctx;
    aes_init(&ctx, 0, key);

    // 分块解密
    for (size_t i = 0; i < ciphertext_len; i += AES_BLOCK_SIZE) {
        aes_process_block(&ctx, ciphertext + i, decrypted + i);
    }

    // 去填充
    *plaintext = aes_unpad_pkcs7(decrypted, ciphertext_len, plaintext_len);
    free(decrypted);

    return (*plaintext != NULL) ? 0 : -1;
}

// AES CBC加密
int aes_encrypt_cbc(const uint8_t* key, const uint8_t* iv, const uint8_t* plaintext, size_t plaintext_len,
                    uint8_t** ciphertext, size_t* ciphertext_len) {
    if (!key || !iv || !plaintext || !ciphertext || !ciphertext_len) return -1;

    // 填充
    size_t padded_len;
    uint8_t* padded = aes_pad_pkcs7(plaintext, plaintext_len, &padded_len);
    if (!padded) return -1;

    *ciphertext_len = padded_len;
    *ciphertext = malloc(*ciphertext_len);
    if (!*ciphertext) {
        free(padded);
        return -1;
    }

    aes_context_t ctx;
    aes_init(&ctx, 1, key);

    uint8_t prev_block[AES_BLOCK_SIZE];
    memcpy(prev_block, iv, AES_BLOCK_SIZE);

    // 分块加密
    for (size_t i = 0; i < padded_len; i += AES_BLOCK_SIZE) {
        uint8_t xor_block[AES_BLOCK_SIZE];

        // XOR with previous block
        for (int j = 0; j < AES_BLOCK_SIZE; j++) {
            xor_block[j] = padded[i + j] ^ prev_block[j];
        }

        aes_process_block(&ctx, xor_block, *ciphertext + i);
        memcpy(prev_block, *ciphertext + i, AES_BLOCK_SIZE);
    }

    free(padded);
    return 0;
}

// AES CBC解密
int aes_decrypt_cbc(const uint8_t* key, const uint8_t* iv, const uint8_t* ciphertext, size_t ciphertext_len,
                    uint8_t** plaintext, size_t* plaintext_len) {
    if (!key || !iv || !ciphertext || !plaintext || !plaintext_len) return -1;

    uint8_t* decrypted = malloc(ciphertext_len);
    if (!decrypted) return -1;

    aes_context_t ctx;
    aes_init(&ctx, 0, key);

    uint8_t prev_block[AES_BLOCK_SIZE];
    memcpy(prev_block, iv, AES_BLOCK_SIZE);

    // 分块解密
    for (size_t i = 0; i < ciphertext_len; i += AES_BLOCK_SIZE) {
        uint8_t temp_block[AES_BLOCK_SIZE];
        memcpy(temp_block, ciphertext + i, AES_BLOCK_SIZE);

        aes_process_block(&ctx, ciphertext + i, decrypted + i);

        // XOR with previous block
        for (int j = 0; j < AES_BLOCK_SIZE; j++) {
            decrypted[i + j] ^= prev_block[j];
        }

        memcpy(prev_block, temp_block, AES_BLOCK_SIZE);
    }

    // 去填充
    *plaintext = aes_unpad_pkcs7(decrypted, ciphertext_len, plaintext_len);
    free(decrypted);

    return (*plaintext != NULL) ? 0 : -1;
}

// 双重AES ECB加密
int aes_double_encrypt_ecb(const uint8_t* key1, const uint8_t* key2, const uint8_t* plaintext, size_t plaintext_len,
                          uint8_t** ciphertext, size_t* ciphertext_len) {
    uint8_t* temp_ciphertext;
    size_t temp_len;

    // 第一次加密
    int result = aes_encrypt_ecb(key1, plaintext, plaintext_len, &temp_ciphertext, &temp_len);
    if (result != 0) return result;

    // 第二次加密
    result = aes_encrypt_ecb(key2, temp_ciphertext, temp_len, ciphertext, ciphertext_len);
    free(temp_ciphertext);

    return result;
}

// 双重AES ECB解密
int aes_double_decrypt_ecb(const uint8_t* key1, const uint8_t* key2, const uint8_t* ciphertext, size_t ciphertext_len,
                          uint8_t** plaintext, size_t* plaintext_len) {
    uint8_t* temp_plaintext;
    size_t temp_len;

    // 第一次解密（使用key2）
    int result = aes_decrypt_ecb(key2, ciphertext, ciphertext_len, &temp_plaintext, &temp_len);
    if (result != 0) return result;

    // 第二次解密（使用key1）
    result = aes_decrypt_ecb(key1, temp_plaintext, temp_len, plaintext, plaintext_len);
    free(temp_plaintext);

    return result;
}

// 双重AES CBC加密
int aes_double_encrypt_cbc(const uint8_t* key1, const uint8_t* key2, const uint8_t* iv,
                          const uint8_t* plaintext, size_t plaintext_len,
                          uint8_t** ciphertext, size_t* ciphertext_len) {
    uint8_t* temp_ciphertext;
    size_t temp_len;

    // 第一次加密
    int result = aes_encrypt_cbc(key1, iv, plaintext, plaintext_len, &temp_ciphertext, &temp_len);
    if (result != 0) return result;

    // 第二次加密（使用第一次加密结果的前16字节作为IV）
    result = aes_encrypt_cbc(key2, temp_ciphertext, temp_ciphertext + AES_BLOCK_SIZE,
                            temp_len - AES_BLOCK_SIZE, ciphertext, ciphertext_len);
    free(temp_ciphertext);

    return result;
}

// 双重AES CBC解密
int aes_double_decrypt_cbc(const uint8_t* key1, const uint8_t* key2, const uint8_t* iv,
                          const uint8_t* ciphertext, size_t ciphertext_len,
                          uint8_t** plaintext, size_t* plaintext_len) {
    uint8_t* temp_plaintext;
    size_t temp_len;

    // 第一次解密（使用key2，从第16字节开始解密）
    int result = aes_decrypt_cbc(key2, ciphertext, ciphertext + AES_BLOCK_SIZE,
                                ciphertext_len - AES_BLOCK_SIZE, &temp_plaintext, &temp_len);
    if (result != 0) return result;

    // 构造完整的中间结果
    uint8_t* full_temp = malloc(temp_len + AES_BLOCK_SIZE);
    if (!full_temp) {
        free(temp_plaintext);
        return -1;
    }

    memcpy(full_temp, ciphertext, AES_BLOCK_SIZE);
    memcpy(full_temp + AES_BLOCK_SIZE, temp_plaintext, temp_len);
    free(temp_plaintext);

    // 第二次解密（使用key1）
    result = aes_decrypt_cbc(key1, iv, full_temp, temp_len + AES_BLOCK_SIZE, plaintext, plaintext_len);
    free(full_temp);

    return result;
}