#include "utils/sim/sim_evp_internal.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static const uint8_t DES_IP[64] = {
    58, 50, 42, 34, 26, 18, 10, 2,
    60, 52, 44, 36, 28, 20, 12, 4,
    62, 54, 46, 38, 30, 22, 14, 6,
    64, 56, 48, 40, 32, 24, 16, 8,
    57, 49, 41, 33, 25, 17, 9, 1,
    59, 51, 43, 35, 27, 19, 11, 3,
    61, 53, 45, 37, 29, 21, 13, 5,
    63, 55, 47, 39, 31, 23, 15, 7
};

static const uint8_t DES_FP[64] = {
    40, 8, 48, 16, 56, 24, 64, 32,
    39, 7, 47, 15, 55, 23, 63, 31,
    38, 6, 46, 14, 54, 22, 62, 30,
    37, 5, 45, 13, 53, 21, 61, 29,
    36, 4, 44, 12, 52, 20, 60, 28,
    35, 3, 43, 11, 51, 19, 59, 27,
    34, 2, 42, 10, 50, 18, 58, 26,
    33, 1, 41, 9, 49, 17, 57, 25
};

static const uint8_t DES_E[48] = {
    32, 1, 2, 3, 4, 5,
    4, 5, 6, 7, 8, 9,
    8, 9, 10, 11, 12, 13,
    12, 13, 14, 15, 16, 17,
    16, 17, 18, 19, 20, 21,
    20, 21, 22, 23, 24, 25,
    24, 25, 26, 27, 28, 29,
    28, 29, 30, 31, 32, 1
};

static const uint8_t DES_P[32] = {
    16, 7, 20, 21, 29, 12, 28, 17,
    1, 15, 23, 26, 5, 18, 31, 10,
    2, 8, 24, 14, 32, 27, 3, 9,
    19, 13, 30, 6, 22, 11, 4, 25
};

static const uint8_t DES_PC1[56] = {
    57, 49, 41, 33, 25, 17, 9,
    1, 58, 50, 42, 34, 26, 18,
    10, 2, 59, 51, 43, 35, 27,
    19, 11, 3, 60, 52, 44, 36,
    63, 55, 47, 39, 31, 23, 15,
    7, 62, 54, 46, 38, 30, 22,
    14, 6, 61, 53, 45, 37, 29,
    21, 13, 5, 28, 20, 12, 4
};

static const uint8_t DES_PC2[48] = {
    14, 17, 11, 24, 1, 5,
    3, 28, 15, 6, 21, 10,
    23, 19, 12, 4, 26, 8,
    16, 7, 27, 20, 13, 2,
    41, 52, 31, 37, 47, 55,
    30, 40, 51, 45, 33, 48,
    44, 49, 39, 56, 34, 53,
    46, 42, 50, 36, 29, 32
};

static const uint8_t DES_SHIFTS[16] = {1, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1};

static const uint8_t DES_SBOX[8][64] = {
    {
        14, 4, 13, 1, 2, 15, 11, 8, 3, 10, 6, 12, 5, 9, 0, 7,
        0, 15, 7, 4, 14, 2, 13, 1, 10, 6, 12, 11, 9, 5, 3, 8,
        4, 1, 14, 8, 13, 6, 2, 11, 15, 12, 9, 7, 3, 10, 5, 0,
        15, 12, 8, 2, 4, 9, 1, 7, 5, 11, 3, 14, 10, 0, 6, 13
    },
    {
        15, 1, 8, 14, 6, 11, 3, 4, 9, 7, 2, 13, 12, 0, 5, 10,
        3, 13, 4, 7, 15, 2, 8, 14, 12, 0, 1, 10, 6, 9, 11, 5,
        0, 14, 7, 11, 10, 4, 13, 1, 5, 8, 12, 6, 9, 3, 2, 15,
        13, 8, 10, 1, 3, 15, 4, 2, 11, 6, 7, 12, 0, 5, 14, 9
    },
    {
        10, 0, 9, 14, 6, 3, 15, 5, 1, 13, 12, 7, 11, 4, 2, 8,
        13, 7, 0, 9, 3, 4, 6, 10, 2, 8, 5, 14, 12, 11, 15, 1,
        13, 6, 4, 9, 8, 15, 3, 0, 11, 1, 2, 12, 5, 10, 14, 7,
        1, 10, 13, 0, 6, 9, 8, 7, 4, 15, 14, 3, 11, 5, 2, 12
    },
    {
        7, 13, 14, 3, 0, 6, 9, 10, 1, 2, 8, 5, 11, 12, 4, 15,
        13, 8, 11, 5, 6, 15, 0, 3, 4, 7, 2, 12, 1, 10, 14, 9,
        10, 6, 9, 0, 12, 11, 7, 13, 15, 1, 3, 14, 5, 2, 8, 4,
        3, 15, 0, 6, 10, 1, 13, 8, 9, 4, 5, 11, 12, 7, 2, 14
    },
    {
        2, 12, 4, 1, 7, 10, 11, 6, 8, 5, 3, 15, 13, 0, 14, 9,
        14, 11, 2, 12, 4, 7, 13, 1, 5, 0, 15, 10, 3, 9, 8, 6,
        4, 2, 1, 11, 10, 13, 7, 8, 15, 9, 12, 5, 6, 3, 0, 14,
        11, 8, 12, 7, 1, 14, 2, 13, 6, 15, 0, 9, 10, 4, 5, 3
    },
    {
        12, 1, 10, 15, 9, 2, 6, 8, 0, 13, 3, 4, 14, 7, 5, 11,
        10, 15, 4, 2, 7, 12, 9, 5, 6, 1, 13, 14, 0, 11, 3, 8,
        9, 14, 15, 5, 2, 8, 12, 3, 7, 0, 4, 10, 1, 13, 11, 6,
        4, 3, 2, 12, 9, 5, 15, 10, 11, 14, 1, 7, 6, 0, 8, 13
    },
    {
        4, 11, 2, 14, 15, 0, 8, 13, 3, 12, 9, 7, 5, 10, 6, 1,
        13, 0, 11, 7, 4, 9, 1, 10, 14, 3, 5, 12, 2, 15, 8, 6,
        1, 4, 11, 13, 12, 3, 7, 14, 10, 15, 6, 8, 0, 5, 9, 2,
        6, 11, 13, 8, 1, 4, 10, 7, 9, 5, 0, 15, 14, 2, 3, 12
    },
    {
        13, 2, 8, 4, 6, 15, 11, 1, 10, 9, 3, 14, 5, 0, 12, 7,
        1, 15, 13, 8, 10, 3, 7, 4, 12, 5, 6, 11, 0, 14, 9, 2,
        7, 11, 4, 1, 9, 12, 14, 2, 0, 6, 10, 13, 15, 3, 5, 8,
        2, 1, 14, 7, 4, 10, 8, 13, 15, 12, 9, 0, 3, 5, 6, 11
    }
};

static uint64_t des_bytes_to_u64(const uint8_t b[8])
{
    uint64_t v = 0;
    int i;
    for (i = 0; i < 8; i++) v = (v << 8) | (uint64_t)b[i];
    return v;
}

static void des_u64_to_bytes(uint64_t v, uint8_t b[8])
{
    int i;
    for (i = 7; i >= 0; i--)
    {
        b[i] = (uint8_t)(v & 0xFF);
        v >>= 8;
    }
}

/*
 * 通用置换：in 是 in_bits 位、按 MSB-first 存放在低 in_bits 位中的值；
 * table 为 1-based 的位置表（第 1 位 = MSB）。输出同样 MSB-first。
 */
static uint64_t des_permute(uint64_t in, int in_bits, const uint8_t* table, int out_bits)
{
    uint64_t out = 0;
    int i;
    for (i = 0; i < out_bits; i++)
    {
        int src = table[i];
        uint64_t bit = (in >> (in_bits - src)) & 1ULL;
        out = (out << 1) | bit;
    }
    return out;
}

void des_key_schedule(const uint8_t key[8], uint64_t subkeys[16])
{
    uint64_t k = des_bytes_to_u64(key);
    uint64_t pc1 = des_permute(k, 64, DES_PC1, 56);
    uint32_t c = (uint32_t)((pc1 >> 28) & 0x0FFFFFFFu);
    uint32_t d = (uint32_t)(pc1 & 0x0FFFFFFFu);
    int i;

    for (i = 0; i < 16; i++)
    {
        int s = DES_SHIFTS[i];
        c = ((c << s) | (c >> (28 - s))) & 0x0FFFFFFFu;
        d = ((d << s) | (d >> (28 - s))) & 0x0FFFFFFFu;
        uint64_t cd = ((uint64_t)c << 28) | (uint64_t)d;
        subkeys[i] = des_permute(cd, 56, DES_PC2, 48);
    }
}

static uint32_t des_feistel(uint32_t r, uint64_t subkey)
{
    uint64_t e = des_permute(r, 32, DES_E, 48);
    uint64_t x = e ^ subkey;
    uint32_t out = 0;
    int i;

    for (i = 0; i < 8; i++)
    {
        int chunk = (int)((x >> (42 - 6 * i)) & 0x3FULL);
        int row = ((chunk & 0x20) >> 4) | (chunk & 0x01);
        int col = (chunk >> 1) & 0x0F;
        out = (out << 4) | (uint32_t)DES_SBOX[i][row * 16 + col];
    }
    return (uint32_t)des_permute(out, 32, DES_P, 32);
}

static void des_crypt_block(const uint64_t subkeys[16], const uint8_t in[8], uint8_t out[8],
                            int decrypt)
{
    uint64_t ip = des_permute(des_bytes_to_u64(in), 64, DES_IP, 64);
    uint32_t l = (uint32_t)(ip >> 32);
    uint32_t r = (uint32_t)(ip & 0xFFFFFFFFu);
    int i;

    for (i = 0; i < 16; i++)
    {
        uint64_t k = decrypt ? subkeys[15 - i] : subkeys[i];
        uint32_t t = l ^ des_feistel(r, k);
        l = r;
        r = t;
    }

    uint64_t pre = ((uint64_t)r << 32) | (uint64_t)l;   /* 末轮后左右交换 */
    des_u64_to_bytes(des_permute(pre, 64, DES_FP, 64), out);
}

void simssl_des3_encrypt_block(const unsigned char key[24], const unsigned char in[8],
                              unsigned char out[8])
{
    uint64_t k1[16], k2[16], k3[16];
    uint8_t t1[8], t2[8];

    des_key_schedule(key, k1);
    des_key_schedule(key + 8, k2);
    des_key_schedule(key + 16, k3);

    des_crypt_block(k1, in, t1, 0);     /* E_K1 */
    des_crypt_block(k2, t1, t2, 1);     /* D_K2 */
    des_crypt_block(k3, t2, out, 0);    /* E_K3 */
}

void simssl_des3_decrypt_block(const unsigned char key[24], const unsigned char in[8],
                              unsigned char out[8])
{
    uint64_t k1[16], k2[16], k3[16];
    uint8_t t1[8], t2[8];

    des_key_schedule(key, k1);
    des_key_schedule(key + 8, k2);
    des_key_schedule(key + 16, k3);

    des_crypt_block(k3, in, t1, 1);     /* D_K3 */
    des_crypt_block(k2, t1, t2, 0);     /* E_K2 */
    des_crypt_block(k1, t2, out, 1);    /* D_K1 */
}

static const EVP_CIPHER g_des_ede3_ecb = {.alg = MYSSL_ALG_DES_EDE3_ECB, .block_size = 8, .key_len = 24, .iv_len = 0, .name = "DES-EDE3-ECB"};
static const EVP_CIPHER g_des_ede3_cbc = {.alg = MYSSL_ALG_DES_EDE3_CBC, .block_size = 8, .key_len = 24, .iv_len = 8, .name = "DES-EDE3-CBC"};

const EVP_CIPHER* EVP_des_ede3_ecb(void) { return &g_des_ede3_ecb; }
const EVP_CIPHER* EVP_des_ede3_cbc(void) { return &g_des_ede3_cbc; }
