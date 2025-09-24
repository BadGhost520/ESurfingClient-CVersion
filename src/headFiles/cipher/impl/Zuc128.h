//
// Created by bad_g on 2025/9/25.
//

#ifndef ESURFINGCLIENT_ZUC128_H
#define ESURFINGCLIENT_ZUC128_H

#include <stdint.h>

#define ZUC_KEY_SIZE 16
#define ZUC_IV_SIZE 16

// ZUC128上下文结构体
typedef struct {
    uint32_t LFSR[16];      // 线性反馈移位寄存器
    uint32_t F[2];          // 非线性函数状态
    uint32_t BRC[4];        // 位重组缓存
    int the_index;          // 当前索引
    uint8_t key_stream[4];  // 密钥流缓存
    int the_iterations;     // 迭代次数
} zuc128_context_t;

// ZUC128函数声明
void zuc128_init(zuc128_context_t* ctx, const uint8_t* key, const uint8_t* iv);
int zuc128_process_bytes(zuc128_context_t* ctx, const uint8_t* input, int input_offset,
                         int length, uint8_t* output, int output_offset);
uint8_t zuc128_return_byte(zuc128_context_t* ctx, uint8_t input);

// 流密码加解密函数
int zuc128_encrypt(const uint8_t* key, const uint8_t* iv, const uint8_t* plaintext,
                   size_t plaintext_len, uint8_t** ciphertext, size_t* ciphertext_len);
int zuc128_decrypt(const uint8_t* key, const uint8_t* iv, const uint8_t* ciphertext,
                   size_t ciphertext_len, uint8_t** plaintext, size_t* plaintext_len);

// 内部函数声明
static void zuc128_encode32be(uint32_t val, uint8_t* buf, size_t off);
static uint32_t zuc128_AddM(uint32_t a, uint32_t b);
static uint32_t zuc128_MulByPow2(uint32_t x, int k);
static void zuc128_LFSRWithInitialisationMode(zuc128_context_t* ctx, uint32_t u);
static void zuc128_LFSRWithWorkMode(zuc128_context_t* ctx);
static void zuc128_BitReorganization(zuc128_context_t* ctx);
static uint32_t zuc128_ROT(uint32_t a, int k);
static uint32_t zuc128_L1(uint32_t X);
static uint32_t zuc128_L2(uint32_t X);
static uint32_t zuc128_MAKEU32(uint8_t a, uint8_t b, uint8_t c, uint8_t d);
static uint32_t zuc128_F_function(zuc128_context_t* ctx);
static uint32_t zuc128_MAKEU31(uint8_t a, uint16_t b, uint8_t c);
static void zuc128_setKeyAndIV(zuc128_context_t* ctx, const uint8_t* k, const uint8_t* iv);
static void zuc128_makeKeyStream(zuc128_context_t* ctx);
static uint32_t zuc128_makeKeyStreamWord(zuc128_context_t* ctx);

#endif //ESURFINGCLIENT_ZUC128_H