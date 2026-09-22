#ifndef ESURFINGCLIENT_SIM_EVP_INTERNAL_H
#define ESURFINGCLIENT_SIM_EVP_INTERNAL_H

#include "utils/sim/SimEvp.h"

#include <stdint.h>

enum
{
    MYSSL_ALG_AES_128_ECB = 1,
    MYSSL_ALG_AES_128_CBC,
    MYSSL_ALG_DES_EDE3_ECB,
    MYSSL_ALG_DES_EDE3_CBC
};

struct myssl_cipher_st
{
    int alg;
    int block_size;
    int key_len;
    int iv_len;
    const char* name;
};

/*
 * 分组算法族的内部接口: 定义在 sim_evp_aes.c / sim_evp_des3.c。
 * EVP 兼容层 (sim_evp_cipher.c) 装配轮密钥与单块加解密要用它们, 因此不能再是 static
 */
void aes128_expand_key(const uint8_t key[16], uint8_t rk[176]);

void aes128_encrypt_block_rk(const uint8_t rk[176], const uint8_t in[16], uint8_t out[16]);

void aes128_decrypt_block_rk(const uint8_t rk[176], const uint8_t in[16], uint8_t out[16]);

void des_key_schedule(const uint8_t key[8], uint64_t subkeys[16]);

#endif
