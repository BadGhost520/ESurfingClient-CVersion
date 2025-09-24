//
// Created by bad_g on 2025/9/25.
//
#include <stdlib.h>
#include <string.h>

#include "../../headFiles/cipher/impl/Zuc128.h"

// ZUC S盒S0
static const uint8_t ZUC_S0[256] = {
    0x3e, 0x72, 0x5b, 0x47, 0xca, 0xe0, 0x00, 0x33, 0x04, 0xd1, 0x54, 0x98, 0x09, 0xb9, 0x6d, 0xcb,
    0x7b, 0x1b, 0xf9, 0x32, 0xaf, 0x9d, 0x6a, 0xa5, 0xb8, 0x2d, 0xfc, 0x1d, 0x08, 0x53, 0x03, 0x90,
    0x4d, 0x4e, 0x84, 0x99, 0xe4, 0xce, 0xd9, 0x91, 0xdd, 0xb6, 0x85, 0x48, 0x8b, 0x29, 0x6e, 0xac,
    0xcd, 0xc1, 0xf8, 0x1e, 0x73, 0x43, 0x69, 0xc6, 0xb5, 0xbd, 0xfd, 0x39, 0x63, 0x20, 0xd4, 0x38,
    0x76, 0x7d, 0xb2, 0xa7, 0xcf, 0xed, 0x57, 0xc5, 0xf3, 0x2c, 0xbb, 0x14, 0x21, 0x06, 0x55, 0x9b,
    0xe3, 0xef, 0x5e, 0x31, 0x4f, 0x7f, 0x5a, 0xa4, 0x0d, 0x82, 0x51, 0x49, 0x5f, 0xba, 0x58, 0x1c,
    0x4a, 0x16, 0xd5, 0x17, 0xa8, 0x92, 0x24, 0x1f, 0x8c, 0xff, 0xd8, 0xae, 0x2e, 0x01, 0xd3, 0xad,
    0x3b, 0x4b, 0xda, 0x46, 0xeb, 0xc9, 0xde, 0x9a, 0x8f, 0x87, 0xd7, 0x3a, 0x80, 0x6f, 0x2f, 0xc8,
    0xb1, 0xb4, 0x37, 0xf7, 0x0a, 0x22, 0x13, 0x28, 0x7c, 0xcc, 0x3c, 0x89, 0xc7, 0xc3, 0x96, 0x56,
    0x07, 0xbf, 0x7e, 0xf0, 0x0b, 0x2b, 0x97, 0x52, 0x35, 0x41, 0x79, 0x61, 0xa6, 0x4c, 0x10, 0xfe,
    0xbc, 0x26, 0x95, 0x88, 0x8a, 0xb0, 0xa3, 0xfb, 0xc0, 0x18, 0x94, 0xf2, 0xe1, 0xe5, 0xe9, 0x5d,
    0xd0, 0xdc, 0x11, 0x66, 0x64, 0x5c, 0xec, 0x59, 0x42, 0x75, 0x12, 0xf5, 0x74, 0x9c, 0xaa, 0x23,
    0x0e, 0x86, 0xab, 0xbe, 0x2a, 0x02, 0xe7, 0x67, 0xe6, 0x44, 0xa2, 0x6c, 0xc2, 0x93, 0x9f, 0xf1,
    0xf6, 0xfa, 0x36, 0xd2, 0x50, 0x68, 0x9e, 0x62, 0x71, 0x15, 0x3d, 0xd6, 0x40, 0xc4, 0xe2, 0x0f,
    0x8e, 0x83, 0x77, 0x6b, 0x25, 0x05, 0x3f, 0x0c, 0x30, 0xea, 0x70, 0xb7, 0xa1, 0xe8, 0xa9, 0x65,
    0x8d, 0x27, 0x1a, 0xdb, 0x81, 0xb3, 0xa0, 0xf4, 0x45, 0x7a, 0x19, 0xdf, 0xee, 0x78, 0x34, 0x60
};

// ZUC S盒S1
static const uint8_t ZUC_S1[256] = {
    0x55, 0xc2, 0x63, 0x71, 0x3b, 0xc8, 0x47, 0x86, 0x9f, 0x3c, 0xda, 0x5b, 0x29, 0xaa, 0xfd, 0x77,
    0x8c, 0xc5, 0x94, 0x0c, 0xa6, 0x1a, 0x13, 0x00, 0xe3, 0xa8, 0x16, 0x72, 0x40, 0xf9, 0xf8, 0x42,
    0x44, 0x26, 0x68, 0x96, 0x81, 0xd9, 0x45, 0x3e, 0x10, 0x76, 0xc6, 0xa7, 0x8b, 0x39, 0x43, 0xe1,
    0x3a, 0xb5, 0x56, 0x2a, 0xc0, 0x6d, 0xb3, 0x05, 0x22, 0x66, 0xbf, 0xdc, 0x0b, 0xfa, 0x62, 0x48,
    0xdd, 0x20, 0x11, 0x06, 0x36, 0xc9, 0xc1, 0xcf, 0xf6, 0x27, 0x52, 0xbb, 0x69, 0xf5, 0xd4, 0x87,
    0x7f, 0x84, 0x4c, 0xd2, 0x9c, 0x57, 0xa4, 0xbc, 0x4f, 0x9a, 0xdf, 0xfe, 0xd6, 0x8d, 0x7a, 0xeb,
    0x2b, 0x53, 0xd8, 0x5c, 0xa1, 0x14, 0x17, 0xfb, 0x23, 0xd5, 0x7d, 0x30, 0x67, 0x73, 0x08, 0x09,
    0xee, 0xb7, 0x70, 0x3f, 0x61, 0xb2, 0x19, 0x8e, 0x4e, 0xe5, 0x4b, 0x93, 0x8f, 0x5d, 0xdb, 0xa9,
    0xad, 0xf1, 0xae, 0x2e, 0xcb, 0x0d, 0xfc, 0xf4, 0x2d, 0x46, 0x6e, 0x1d, 0x97, 0xe8, 0xd1, 0xe9,
    0x4d, 0x37, 0xa5, 0x75, 0x5e, 0x83, 0x9e, 0xab, 0x82, 0x9d, 0xb9, 0x1c, 0xe0, 0xcd, 0x49, 0x89,
    0x01, 0xb6, 0xbd, 0x58, 0x24, 0xa2, 0x5f, 0x38, 0x78, 0x99, 0x15, 0x90, 0x50, 0xb8, 0x95, 0xe4,
    0xd0, 0x91, 0xc7, 0xce, 0xed, 0x0f, 0xb4, 0x6f, 0xa0, 0xcc, 0xf0, 0x02, 0x4a, 0x79, 0xc3, 0xde,
    0xa3, 0xef, 0xea, 0x51, 0xe6, 0x6b, 0x18, 0xec, 0x1b, 0x2c, 0x80, 0xf7, 0x74, 0xe7, 0xff, 0x21,
    0x5a, 0x6a, 0x54, 0x1e, 0x41, 0x31, 0x92, 0x35, 0xc4, 0x33, 0x07, 0x0a, 0xba, 0x7e, 0x0e, 0x34,
    0x88, 0xb1, 0x98, 0x7c, 0xf3, 0x3d, 0x60, 0x6c, 0x7b, 0xca, 0xd3, 0x1f, 0x32, 0x65, 0x04, 0x28,
    0x64, 0xbe, 0x85, 0x9b, 0x2f, 0x59, 0x8a, 0xd7, 0xb0, 0x25, 0xac, 0xaf, 0x12, 0x03, 0xe2, 0xf2
};

// ZUC常数D
static const uint16_t ZUC_EK_d[16] = {
    0x44D7, 0x26BC, 0x626B, 0x135E, 0x5789, 0x35E2, 0x7135, 0x09AF,
    0x4D78, 0x2F13, 0x6BC4, 0x1AF1, 0x5E26, 0x3C4D, 0x789A, 0x47AC
};

// 编码32位大端序
static void zuc128_encode32be(uint32_t val, uint8_t* buf, size_t off) {
    buf[off] = (val >> 24) & 0xFF;
    buf[off + 1] = (val >> 16) & 0xFF;
    buf[off + 2] = (val >> 8) & 0xFF;
    buf[off + 3] = val & 0xFF;
}

// 模2^31-1加法
static uint32_t zuc128_AddM(uint32_t a, uint32_t b) {
    uint32_t c = a + b;
    return (c & 0x7FFFFFFF) + (c >> 31);
}

// 乘以2的k次方
static uint32_t zuc128_MulByPow2(uint32_t x, int k) {
    return ((x << k) | (x >> (31 - k))) & 0x7FFFFFFF;
}

// 初始化模式的LFSR
static void zuc128_LFSRWithInitialisationMode(zuc128_context_t* ctx, uint32_t u) {
    uint32_t f = ctx->LFSR[0];
    uint32_t v = zuc128_MulByPow2(ctx->LFSR[0], 8);
    f = zuc128_AddM(f, v);
    v = zuc128_MulByPow2(ctx->LFSR[4], 20);
    f = zuc128_AddM(f, v);
    v = zuc128_MulByPow2(ctx->LFSR[10], 21);
    f = zuc128_AddM(f, v);
    v = zuc128_MulByPow2(ctx->LFSR[13], 17);
    f = zuc128_AddM(f, v);
    v = zuc128_MulByPow2(ctx->LFSR[15], 15);
    f = zuc128_AddM(f, v);
    f = zuc128_AddM(f, u);

    // 移位
    for (int i = 0; i < 15; i++) {
        ctx->LFSR[i] = ctx->LFSR[i + 1];
    }
    ctx->LFSR[15] = f;
}

// 工作模式的LFSR
static void zuc128_LFSRWithWorkMode(zuc128_context_t* ctx) {
    uint32_t f = ctx->LFSR[0];
    uint32_t v = zuc128_MulByPow2(ctx->LFSR[0], 8);
    f = zuc128_AddM(f, v);
    v = zuc128_MulByPow2(ctx->LFSR[4], 20);
    f = zuc128_AddM(f, v);
    v = zuc128_MulByPow2(ctx->LFSR[10], 21);
    f = zuc128_AddM(f, v);
    v = zuc128_MulByPow2(ctx->LFSR[13], 17);
    f = zuc128_AddM(f, v);
    v = zuc128_MulByPow2(ctx->LFSR[15], 15);
    f = zuc128_AddM(f, v);

    // 移位
    for (int i = 0; i < 15; i++) {
        ctx->LFSR[i] = ctx->LFSR[i + 1];
    }
    ctx->LFSR[15] = f;
}

// 位重组
static void zuc128_BitReorganization(zuc128_context_t* ctx) {
    ctx->BRC[0] = ((ctx->LFSR[15] & 0x7FFF8000) << 1) | (ctx->LFSR[14] & 0xFFFF);
    ctx->BRC[1] = ((ctx->LFSR[11] & 0xFFFF) << 16) | (ctx->LFSR[9] >> 15);
    ctx->BRC[2] = ((ctx->LFSR[7] & 0xFFFF) << 16) | (ctx->LFSR[5] >> 15);
    ctx->BRC[3] = ((ctx->LFSR[2] & 0xFFFF) << 16) | (ctx->LFSR[0] >> 15);
}

// 循环左移
static uint32_t zuc128_ROT(uint32_t a, int k) {
    return (a << k) | (a >> (32 - k));
}

// L1变换
static uint32_t zuc128_L1(uint32_t X) {
    return X ^ zuc128_ROT(X, 2) ^ zuc128_ROT(X, 10) ^ zuc128_ROT(X, 18) ^ zuc128_ROT(X, 24);
}

// L2变换
static uint32_t zuc128_L2(uint32_t X) {
    return X ^ zuc128_ROT(X, 8) ^ zuc128_ROT(X, 14) ^ zuc128_ROT(X, 22) ^ zuc128_ROT(X, 30);
}

// 构造32位数
static uint32_t zuc128_MAKEU32(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    return ((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)c << 8) | (uint32_t)d;
}

// 非线性函数F
static uint32_t zuc128_F_function(zuc128_context_t* ctx) {
    uint32_t W = (ctx->BRC[0] ^ ctx->F[0]) + ctx->F[1];
    uint32_t W1 = ctx->F[0] + ctx->BRC[1];
    uint32_t W2 = ctx->F[1] ^ ctx->BRC[2];

    uint32_t u = zuc128_L1((W1 << 16) | (W2 >> 16));
    uint32_t v = zuc128_L2((W2 << 16) | (W1 >> 16));

    ctx->F[0] = zuc128_MAKEU32(ZUC_S0[u >> 24], ZUC_S1[(u >> 16) & 0xFF],
                               ZUC_S0[(u >> 8) & 0xFF], ZUC_S1[u & 0xFF]);
    ctx->F[1] = zuc128_MAKEU32(ZUC_S0[v >> 24], ZUC_S1[(v >> 16) & 0xFF],
                               ZUC_S0[(v >> 8) & 0xFF], ZUC_S1[v & 0xFF]);

    return W;
}

// 构造31位数
static uint32_t zuc128_MAKEU31(uint8_t a, uint16_t b, uint8_t c) {
    return ((uint32_t)a << 23) | ((uint32_t)b << 8) | (uint32_t)c;
}

// 设置密钥和IV
static void zuc128_setKeyAndIV(zuc128_context_t* ctx, const uint8_t* k, const uint8_t* iv) {
    // 初始化LFSR
    for (int i = 0; i < 16; i++) {
        ctx->LFSR[i] = zuc128_MAKEU31(k[i], ZUC_EK_d[i], iv[i]);
    }

    // 初始化F
    ctx->F[0] = 0;
    ctx->F[1] = 0;

    // 32轮初始化
    for (int i = 0; i < 32; i++) {
        zuc128_BitReorganization(ctx);
        uint32_t w = zuc128_F_function(ctx);
        zuc128_LFSRWithInitialisationMode(ctx, w >> 1);
    }

    // 切换到工作模式
    zuc128_BitReorganization(ctx);
    zuc128_F_function(ctx);
    zuc128_LFSRWithWorkMode(ctx);
}

// 生成密钥流
static void zuc128_makeKeyStream(zuc128_context_t* ctx) {
    zuc128_BitReorganization(ctx);
    uint32_t z = zuc128_F_function(ctx) ^ ctx->BRC[3];
    zuc128_LFSRWithWorkMode(ctx);
    zuc128_encode32be(z, ctx->key_stream, 0);
    ctx->the_index = 0;
}

// 生成密钥流字
static uint32_t zuc128_makeKeyStreamWord(zuc128_context_t* ctx) {
    zuc128_BitReorganization(ctx);
    uint32_t z = zuc128_F_function(ctx) ^ ctx->BRC[3];
    zuc128_LFSRWithWorkMode(ctx);
    return z;
}

// 初始化ZUC128
void zuc128_init(zuc128_context_t* ctx, const uint8_t* key, const uint8_t* iv) {
    memset(ctx, 0, sizeof(zuc128_context_t));
    ctx->the_iterations = 0;
    zuc128_setKeyAndIV(ctx, key, iv);
}

// 处理字节流
int zuc128_process_bytes(zuc128_context_t* ctx, const uint8_t* input, int input_offset,
                         int length, uint8_t* output, int output_offset) {
    if (ctx->the_iterations >= (1 << 20)) {
        return -1; // 超过最大迭代次数
    }

    for (int i = 0; i < length; i++) {
        if (ctx->the_index >= 4) {
            zuc128_makeKeyStream(ctx);
            ctx->the_iterations++;
            if (ctx->the_iterations >= (1 << 20)) {
                return -1;
            }
        }
        output[output_offset + i] = input[input_offset + i] ^ ctx->key_stream[ctx->the_index++];
    }

    return 0;
}

// 处理单个字节
uint8_t zuc128_return_byte(zuc128_context_t* ctx, uint8_t input) {
    if (ctx->the_index >= 4) {
        zuc128_makeKeyStream(ctx);
        ctx->the_iterations++;
    }
    return input ^ ctx->key_stream[ctx->the_index++];
}

// ZUC128加密
int zuc128_encrypt(const uint8_t* key, const uint8_t* iv, const uint8_t* plaintext,
                   size_t plaintext_len, uint8_t** ciphertext, size_t* ciphertext_len) {
    if (!key || !iv || !plaintext || !ciphertext || !ciphertext_len) return -1;

    // 填充到4字节对齐
    size_t padded_len = ((plaintext_len + 3) / 4) * 4;
    uint8_t* padded_input = calloc(padded_len, 1);
    if (!padded_input) return -1;

    memcpy(padded_input, plaintext, plaintext_len);

    *ciphertext_len = padded_len;
    *ciphertext = malloc(*ciphertext_len);
    if (!*ciphertext) {
        free(padded_input);
        return -1;
    }

    zuc128_context_t ctx;
    zuc128_init(&ctx, key, iv);

    int result = zuc128_process_bytes(&ctx, padded_input, 0, (int)padded_len, *ciphertext, 0);
    free(padded_input);

    return result;
}

// ZUC128解密
int zuc128_decrypt(const uint8_t* key, const uint8_t* iv, const uint8_t* ciphertext,
                   size_t ciphertext_len, uint8_t** plaintext, size_t* plaintext_len) {
    if (!key || !iv || !ciphertext || !plaintext || !plaintext_len) return -1;

    uint8_t* decrypted = malloc(ciphertext_len);
    if (!decrypted) return -1;

    zuc128_context_t ctx;
    zuc128_init(&ctx, key, iv);

    int result = zuc128_process_bytes(&ctx, ciphertext, 0, (int)ciphertext_len, decrypted, 0);
    if (result != 0) {
        free(decrypted);
        return result;
    }

    // 去除尾部的零填充
    size_t actual_len = ciphertext_len;
    while (actual_len > 0 && decrypted[actual_len - 1] == 0) {
        actual_len--;
    }

    *plaintext_len = actual_len;
    *plaintext = malloc(*plaintext_len);
    if (!*plaintext) {
        free(decrypted);
        return -1;
    }

    memcpy(*plaintext, decrypted, *plaintext_len);
    free(decrypted);

    return 0;
}