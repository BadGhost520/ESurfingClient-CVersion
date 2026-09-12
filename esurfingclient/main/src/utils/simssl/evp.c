#include "utils/simssl/evp.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ==========================================================================
 * 一、MD5（RFC 1321）
 * ========================================================================== */

#define MYSSL_ROTL32(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

static const uint32_t MD5_T[64] = {
    0xd76aa478u, 0xe8c7b756u, 0x242070dbu, 0xc1bdceeeu,
    0xf57c0fafu, 0x4787c62au, 0xa8304613u, 0xfd469501u,
    0x698098d8u, 0x8b44f7afu, 0xffff5bb1u, 0x895cd7beu,
    0x6b901122u, 0xfd987193u, 0xa679438eu, 0x49b40821u,
    0xf61e2562u, 0xc040b340u, 0x265e5a51u, 0xe9b6c7aau,
    0xd62f105du, 0x02441453u, 0xd8a1e681u, 0xe7d3fbc8u,
    0x21e1cde6u, 0xc33707d6u, 0xf4d50d87u, 0x455a14edu,
    0xa9e3e905u, 0xfcefa3f8u, 0x676f02d9u, 0x8d2a4c8au,
    0xfffa3942u, 0x8771f681u, 0x6d9d6122u, 0xfde5380cu,
    0xa4beea44u, 0x4bdecfa9u, 0xf6bb4b60u, 0xbebfbc70u,
    0x289b7ec6u, 0xeaa127fau, 0xd4ef3085u, 0x04881d05u,
    0xd9d4d039u, 0xe6db99e5u, 0x1fa27cf8u, 0xc4ac5665u,
    0xf4292244u, 0x432aff97u, 0xab9423a7u, 0xfc93a039u,
    0x655b59c3u, 0x8f0ccc92u, 0xffeff47du, 0x85845dd1u,
    0x6fa87e4fu, 0xfe2ce6e0u, 0xa3014314u, 0x4e0811a1u,
    0xf7537e82u, 0xbd3af235u, 0x2ad7d2bbu, 0xeb86d391u
};

#define MD5_F(x, y, z) (((x) & (y)) | (~(x) & (z)))
#define MD5_G(x, y, z) (((x) & (z)) | ((y) & ~(z)))
#define MD5_H(x, y, z) ((x) ^ (y) ^ (z))
#define MD5_I(x, y, z) ((y) ^ ((x) | ~(z)))

#define MD5_STEP(fn, a, b, c, d, x, t, s)      \
    do {                                       \
        (a) += fn((b), (c), (d)) + (x) + (t);  \
        (a) = MYSSL_ROTL32((a), (s));          \
        (a) += (b);                            \
    } while (0)

static void md5_transform(uint32_t state[4], const uint8_t block[64])
{
    uint32_t x[16];
    uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
    int i;

    for (i = 0; i < 16; i++)
        x[i] = (uint32_t)block[i * 4] |
               ((uint32_t)block[i * 4 + 1] << 8) |
               ((uint32_t)block[i * 4 + 2] << 16) |
               ((uint32_t)block[i * 4 + 3] << 24);

    /* Round 1 */
    MD5_STEP(MD5_F, a, b, c, d, x[0],  MD5_T[0],  7);
    MD5_STEP(MD5_F, d, a, b, c, x[1],  MD5_T[1],  12);
    MD5_STEP(MD5_F, c, d, a, b, x[2],  MD5_T[2],  17);
    MD5_STEP(MD5_F, b, c, d, a, x[3],  MD5_T[3],  22);
    MD5_STEP(MD5_F, a, b, c, d, x[4],  MD5_T[4],  7);
    MD5_STEP(MD5_F, d, a, b, c, x[5],  MD5_T[5],  12);
    MD5_STEP(MD5_F, c, d, a, b, x[6],  MD5_T[6],  17);
    MD5_STEP(MD5_F, b, c, d, a, x[7],  MD5_T[7],  22);
    MD5_STEP(MD5_F, a, b, c, d, x[8],  MD5_T[8],  7);
    MD5_STEP(MD5_F, d, a, b, c, x[9],  MD5_T[9],  12);
    MD5_STEP(MD5_F, c, d, a, b, x[10], MD5_T[10], 17);
    MD5_STEP(MD5_F, b, c, d, a, x[11], MD5_T[11], 22);
    MD5_STEP(MD5_F, a, b, c, d, x[12], MD5_T[12], 7);
    MD5_STEP(MD5_F, d, a, b, c, x[13], MD5_T[13], 12);
    MD5_STEP(MD5_F, c, d, a, b, x[14], MD5_T[14], 17);
    MD5_STEP(MD5_F, b, c, d, a, x[15], MD5_T[15], 22);

    /* Round 2 */
    MD5_STEP(MD5_G, a, b, c, d, x[1],  MD5_T[16], 5);
    MD5_STEP(MD5_G, d, a, b, c, x[6],  MD5_T[17], 9);
    MD5_STEP(MD5_G, c, d, a, b, x[11], MD5_T[18], 14);
    MD5_STEP(MD5_G, b, c, d, a, x[0],  MD5_T[19], 20);
    MD5_STEP(MD5_G, a, b, c, d, x[5],  MD5_T[20], 5);
    MD5_STEP(MD5_G, d, a, b, c, x[10], MD5_T[21], 9);
    MD5_STEP(MD5_G, c, d, a, b, x[15], MD5_T[22], 14);
    MD5_STEP(MD5_G, b, c, d, a, x[4],  MD5_T[23], 20);
    MD5_STEP(MD5_G, a, b, c, d, x[9],  MD5_T[24], 5);
    MD5_STEP(MD5_G, d, a, b, c, x[14], MD5_T[25], 9);
    MD5_STEP(MD5_G, c, d, a, b, x[3],  MD5_T[26], 14);
    MD5_STEP(MD5_G, b, c, d, a, x[8],  MD5_T[27], 20);
    MD5_STEP(MD5_G, a, b, c, d, x[13], MD5_T[28], 5);
    MD5_STEP(MD5_G, d, a, b, c, x[2],  MD5_T[29], 9);
    MD5_STEP(MD5_G, c, d, a, b, x[7],  MD5_T[30], 14);
    MD5_STEP(MD5_G, b, c, d, a, x[12], MD5_T[31], 20);

    /* Round 3 */
    MD5_STEP(MD5_H, a, b, c, d, x[5],  MD5_T[32], 4);
    MD5_STEP(MD5_H, d, a, b, c, x[8],  MD5_T[33], 11);
    MD5_STEP(MD5_H, c, d, a, b, x[11], MD5_T[34], 16);
    MD5_STEP(MD5_H, b, c, d, a, x[14], MD5_T[35], 23);
    MD5_STEP(MD5_H, a, b, c, d, x[1],  MD5_T[36], 4);
    MD5_STEP(MD5_H, d, a, b, c, x[4],  MD5_T[37], 11);
    MD5_STEP(MD5_H, c, d, a, b, x[7],  MD5_T[38], 16);
    MD5_STEP(MD5_H, b, c, d, a, x[10], MD5_T[39], 23);
    MD5_STEP(MD5_H, a, b, c, d, x[13], MD5_T[40], 4);
    MD5_STEP(MD5_H, d, a, b, c, x[0],  MD5_T[41], 11);
    MD5_STEP(MD5_H, c, d, a, b, x[3],  MD5_T[42], 16);
    MD5_STEP(MD5_H, b, c, d, a, x[6],  MD5_T[43], 23);
    MD5_STEP(MD5_H, a, b, c, d, x[9],  MD5_T[44], 4);
    MD5_STEP(MD5_H, d, a, b, c, x[12], MD5_T[45], 11);
    MD5_STEP(MD5_H, c, d, a, b, x[15], MD5_T[46], 16);
    MD5_STEP(MD5_H, b, c, d, a, x[2],  MD5_T[47], 23);

    /* Round 4 */
    MD5_STEP(MD5_I, a, b, c, d, x[0],  MD5_T[48], 6);
    MD5_STEP(MD5_I, d, a, b, c, x[7],  MD5_T[49], 10);
    MD5_STEP(MD5_I, c, d, a, b, x[14], MD5_T[50], 15);
    MD5_STEP(MD5_I, b, c, d, a, x[5],  MD5_T[51], 21);
    MD5_STEP(MD5_I, a, b, c, d, x[12], MD5_T[52], 6);
    MD5_STEP(MD5_I, d, a, b, c, x[3],  MD5_T[53], 10);
    MD5_STEP(MD5_I, c, d, a, b, x[10], MD5_T[54], 15);
    MD5_STEP(MD5_I, b, c, d, a, x[1],  MD5_T[55], 21);
    MD5_STEP(MD5_I, a, b, c, d, x[8],  MD5_T[56], 6);
    MD5_STEP(MD5_I, d, a, b, c, x[15], MD5_T[57], 10);
    MD5_STEP(MD5_I, c, d, a, b, x[6],  MD5_T[58], 15);
    MD5_STEP(MD5_I, b, c, d, a, x[13], MD5_T[59], 21);
    MD5_STEP(MD5_I, a, b, c, d, x[4],  MD5_T[60], 6);
    MD5_STEP(MD5_I, d, a, b, c, x[11], MD5_T[61], 10);
    MD5_STEP(MD5_I, c, d, a, b, x[2],  MD5_T[62], 15);
    MD5_STEP(MD5_I, b, c, d, a, x[9],  MD5_T[63], 21);

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
}

/* ==========================================================================
 * 二、AES-128（FIPS-197）
 * ========================================================================== */

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

static const uint8_t AES_RSBOX[256] = {
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

/* GF(2^8) 乘法，模 x^8 + x^4 + x^3 + x + 1 (0x11b) */
static uint8_t aes_gmul(uint8_t a, uint8_t b)
{
    uint8_t p = 0;
    int i;
    for (i = 0; i < 8; i++)
    {
        if (b & 1) p ^= a;
        uint8_t hi = (uint8_t)(a & 0x80);
        a = (uint8_t)(a << 1);
        if (hi) a ^= 0x1b;
        b = (uint8_t)(b >> 1);
    }
    return p;
}

static void aes128_expand_key(const uint8_t key[16], uint8_t rk[176])
{
    int i, j;
    uint8_t rcon = 0x01;

    memcpy(rk, key, 16);
    for (i = 16; i < 176; i += 4)
    {
        uint8_t t[4];
        memcpy(t, rk + i - 4, 4);
        if (i % 16 == 0)
        {
            uint8_t tmp = t[0];
            t[0] = (uint8_t)(AES_SBOX[t[1]] ^ rcon);
            t[1] = AES_SBOX[t[2]];
            t[2] = AES_SBOX[t[3]];
            t[3] = AES_SBOX[tmp];
            rcon = aes_gmul(rcon, 0x02);
        }
        for (j = 0; j < 4; j++) rk[i + j] = (uint8_t)(rk[i - 16 + j] ^ t[j]);
    }
}

static void aes_add_round_key(uint8_t s[16], const uint8_t* rk)
{
    int i;
    for (i = 0; i < 16; i++) s[i] ^= rk[i];
}

static void aes_sub_bytes(uint8_t s[16])
{
    int i;
    for (i = 0; i < 16; i++) s[i] = AES_SBOX[s[i]];
}

static void aes_inv_sub_bytes(uint8_t s[16])
{
    int i;
    for (i = 0; i < 16; i++) s[i] = AES_RSBOX[s[i]];
}

/* 状态按列优先存放：s[r + 4*c]，r 为行、c 为列 */
static void aes_shift_rows(uint8_t s[16])
{
    uint8_t t[16];
    int r, c;
    for (r = 0; r < 4; r++)
        for (c = 0; c < 4; c++)
            t[r + 4 * c] = s[r + 4 * ((c + r) & 3)];
    memcpy(s, t, 16);
}

static void aes_inv_shift_rows(uint8_t s[16])
{
    uint8_t t[16];
    int r, c;
    for (r = 0; r < 4; r++)
        for (c = 0; c < 4; c++)
            t[r + 4 * c] = s[r + 4 * ((c - r + 4) & 3)];
    memcpy(s, t, 16);
}

static void aes_mix_columns(uint8_t s[16])
{
    int c;
    for (c = 0; c < 4; c++)
    {
        uint8_t* p = s + 4 * c;
        uint8_t a0 = p[0], a1 = p[1], a2 = p[2], a3 = p[3];
        p[0] = (uint8_t)(aes_gmul(a0, 2) ^ aes_gmul(a1, 3) ^ a2 ^ a3);
        p[1] = (uint8_t)(a0 ^ aes_gmul(a1, 2) ^ aes_gmul(a2, 3) ^ a3);
        p[2] = (uint8_t)(a0 ^ a1 ^ aes_gmul(a2, 2) ^ aes_gmul(a3, 3));
        p[3] = (uint8_t)(aes_gmul(a0, 3) ^ a1 ^ a2 ^ aes_gmul(a3, 2));
    }
}

static void aes_inv_mix_columns(uint8_t s[16])
{
    int c;
    for (c = 0; c < 4; c++)
    {
        uint8_t* p = s + 4 * c;
        uint8_t a0 = p[0], a1 = p[1], a2 = p[2], a3 = p[3];
        p[0] = (uint8_t)(aes_gmul(a0, 14) ^ aes_gmul(a1, 11) ^ aes_gmul(a2, 13) ^ aes_gmul(a3, 9));
        p[1] = (uint8_t)(aes_gmul(a0, 9) ^ aes_gmul(a1, 14) ^ aes_gmul(a2, 11) ^ aes_gmul(a3, 13));
        p[2] = (uint8_t)(aes_gmul(a0, 13) ^ aes_gmul(a1, 9) ^ aes_gmul(a2, 14) ^ aes_gmul(a3, 11));
        p[3] = (uint8_t)(aes_gmul(a0, 11) ^ aes_gmul(a1, 13) ^ aes_gmul(a2, 9) ^ aes_gmul(a3, 14));
    }
}

static void aes128_encrypt_block_rk(const uint8_t rk[176], const uint8_t in[16], uint8_t out[16])
{
    uint8_t s[16];
    int round;

    memcpy(s, in, 16);
    aes_add_round_key(s, rk);
    for (round = 1; round <= 9; round++)
    {
        aes_sub_bytes(s);
        aes_shift_rows(s);
        aes_mix_columns(s);
        aes_add_round_key(s, rk + 16 * round);
    }
    aes_sub_bytes(s);
    aes_shift_rows(s);
    aes_add_round_key(s, rk + 160);
    memcpy(out, s, 16);
}

static void aes128_decrypt_block_rk(const uint8_t rk[176], const uint8_t in[16], uint8_t out[16])
{
    uint8_t s[16];
    int round;

    memcpy(s, in, 16);
    aes_add_round_key(s, rk + 160);
    for (round = 9; round >= 1; round--)
    {
        aes_inv_shift_rows(s);
        aes_inv_sub_bytes(s);
        aes_add_round_key(s, rk + 16 * round);
        aes_inv_mix_columns(s);
    }
    aes_inv_shift_rows(s);
    aes_inv_sub_bytes(s);
    aes_add_round_key(s, rk);
    memcpy(out, s, 16);
}

void simssl_aes128_encrypt_block(const unsigned char key[16], const unsigned char in[16],
                                unsigned char out[16])
{
    uint8_t rk[176];
    aes128_expand_key(key, rk);
    aes128_encrypt_block_rk(rk, in, out);
}

void simssl_aes128_decrypt_block(const unsigned char key[16], const unsigned char in[16],
                                unsigned char out[16])
{
    uint8_t rk[176];
    aes128_expand_key(key, rk);
    aes128_decrypt_block_rk(rk, in, out);
}

/* ==========================================================================
 * 三、DES / 3DES-EDE3（FIPS 46-3）
 * 位序约定：以「高位为第 1 位」的 MSB-first 方式与标准置换表对应。
 * ========================================================================== */

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
    {   /* S1 */
        14, 4, 13, 1, 2, 15, 11, 8, 3, 10, 6, 12, 5, 9, 0, 7,
        0, 15, 7, 4, 14, 2, 13, 1, 10, 6, 12, 11, 9, 5, 3, 8,
        4, 1, 14, 8, 13, 6, 2, 11, 15, 12, 9, 7, 3, 10, 5, 0,
        15, 12, 8, 2, 4, 9, 1, 7, 5, 11, 3, 14, 10, 0, 6, 13
    },
    {   /* S2 */
        15, 1, 8, 14, 6, 11, 3, 4, 9, 7, 2, 13, 12, 0, 5, 10,
        3, 13, 4, 7, 15, 2, 8, 14, 12, 0, 1, 10, 6, 9, 11, 5,
        0, 14, 7, 11, 10, 4, 13, 1, 5, 8, 12, 6, 9, 3, 2, 15,
        13, 8, 10, 1, 3, 15, 4, 2, 11, 6, 7, 12, 0, 5, 14, 9
    },
    {   /* S3 */
        10, 0, 9, 14, 6, 3, 15, 5, 1, 13, 12, 7, 11, 4, 2, 8,
        13, 7, 0, 9, 3, 4, 6, 10, 2, 8, 5, 14, 12, 11, 15, 1,
        13, 6, 4, 9, 8, 15, 3, 0, 11, 1, 2, 12, 5, 10, 14, 7,
        1, 10, 13, 0, 6, 9, 8, 7, 4, 15, 14, 3, 11, 5, 2, 12
    },
    {   /* S4 */
        7, 13, 14, 3, 0, 6, 9, 10, 1, 2, 8, 5, 11, 12, 4, 15,
        13, 8, 11, 5, 6, 15, 0, 3, 4, 7, 2, 12, 1, 10, 14, 9,
        10, 6, 9, 0, 12, 11, 7, 13, 15, 1, 3, 14, 5, 2, 8, 4,
        3, 15, 0, 6, 10, 1, 13, 8, 9, 4, 5, 11, 12, 7, 2, 14
    },
    {   /* S5 */
        2, 12, 4, 1, 7, 10, 11, 6, 8, 5, 3, 15, 13, 0, 14, 9,
        14, 11, 2, 12, 4, 7, 13, 1, 5, 0, 15, 10, 3, 9, 8, 6,
        4, 2, 1, 11, 10, 13, 7, 8, 15, 9, 12, 5, 6, 3, 0, 14,
        11, 8, 12, 7, 1, 14, 2, 13, 6, 15, 0, 9, 10, 4, 5, 3
    },
    {   /* S6 */
        12, 1, 10, 15, 9, 2, 6, 8, 0, 13, 3, 4, 14, 7, 5, 11,
        10, 15, 4, 2, 7, 12, 9, 5, 6, 1, 13, 14, 0, 11, 3, 8,
        9, 14, 15, 5, 2, 8, 12, 3, 7, 0, 4, 10, 1, 13, 11, 6,
        4, 3, 2, 12, 9, 5, 15, 10, 11, 14, 1, 7, 6, 0, 8, 13
    },
    {   /* S7 */
        4, 11, 2, 14, 15, 0, 8, 13, 3, 12, 9, 7, 5, 10, 6, 1,
        13, 0, 11, 7, 4, 9, 1, 10, 14, 3, 5, 12, 2, 15, 8, 6,
        1, 4, 11, 13, 12, 3, 7, 14, 10, 15, 6, 8, 0, 5, 9, 2,
        6, 11, 13, 8, 1, 4, 10, 7, 9, 5, 0, 15, 14, 2, 3, 12
    },
    {   /* S8 */
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

static void des_key_schedule(const uint8_t key[8], uint64_t subkeys[16])
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

/* ==========================================================================
 * 四、EVP 兼容层
 * ========================================================================== */

enum
{
    MYSSL_ALG_AES_128_ECB = 1,
    MYSSL_ALG_AES_128_CBC,
    MYSSL_ALG_DES_EDE3_ECB,
    MYSSL_ALG_DES_EDE3_CBC
};

struct myssl_md_st
{
    int id;
};

struct myssl_cipher_st
{
    int alg;
    int block_size;
    int key_len;
    int iv_len;
    const char* name;
};

struct myssl_md_ctx_st
{
    uint32_t state[4];
    uint64_t count;
    uint8_t buffer[64];
};

struct myssl_cipher_ctx_st
{
    const EVP_CIPHER* cipher;
    int enc;
    int padding;
    uint8_t key[24];
    uint8_t iv[16];
    uint8_t chain[16];
    uint8_t buf[16];
    size_t buflen;
    uint8_t hold[16];   /* 解密 + padding 时保留的最后一个明文块 */
    int have_hold;
    uint8_t rk[176];    /* AES 轮密钥 */
    uint64_t dk[3][16]; /* 3DES 三组子密钥 */
};

static const EVP_MD g_md5 = {1};
static const EVP_CIPHER g_aes_128_ecb = {.alg = MYSSL_ALG_AES_128_ECB, .block_size = 16, .key_len = 16, .iv_len = 0, .name = "AES-128-ECB"};
static const EVP_CIPHER g_aes_128_cbc = {.alg = MYSSL_ALG_AES_128_CBC, .block_size = 16, .key_len = 16, .iv_len = 16, .name = "AES-128-CBC"};
static const EVP_CIPHER g_des_ede3_ecb = {.alg = MYSSL_ALG_DES_EDE3_ECB, .block_size = 8, .key_len = 24, .iv_len = 0, .name = "DES-EDE3-ECB"};
static const EVP_CIPHER g_des_ede3_cbc = {.alg = MYSSL_ALG_DES_EDE3_CBC, .block_size = 8, .key_len = 24, .iv_len = 8, .name = "DES-EDE3-CBC"};

/* ------------------------------ 摘要 ------------------------------ */

const EVP_MD* EVP_md5(void) { return &g_md5; }

EVP_MD_CTX* EVP_MD_CTX_new(void)
{
    EVP_MD_CTX* ctx = (EVP_MD_CTX*)malloc(sizeof(EVP_MD_CTX));
    if (ctx) memset(ctx, 0, sizeof(*ctx));
    return ctx;
}

void EVP_MD_CTX_free(EVP_MD_CTX* ctx)
{
    if (ctx) free(ctx);
}

int EVP_DigestInit_ex(EVP_MD_CTX* ctx, const EVP_MD* type, void* impl)
{
    (void)impl;
    if (!ctx || !type) return 0;
    ctx->state[0] = 0x67452301u;
    ctx->state[1] = 0xefcdab89u;
    ctx->state[2] = 0x98badcfeu;
    ctx->state[3] = 0x10325476u;
    ctx->count = 0;
    memset(ctx->buffer, 0, sizeof(ctx->buffer));
    return 1;
}

int EVP_DigestUpdate(EVP_MD_CTX* ctx, const void* data, size_t len)
{
    const uint8_t* p = (const uint8_t*)data;
    size_t idx;

    if (!ctx) return 0;
    if (len == 0) return 1;
    if (!p) return 0;

    idx = (size_t)(ctx->count % 64);
    ctx->count += len;

    if (idx + len >= 64)
    {
        size_t part = 64 - idx;
        memcpy(ctx->buffer + idx, p, part);
        md5_transform(ctx->state, ctx->buffer);
        p += part;
        len -= part;
        while (len >= 64)
        {
            md5_transform(ctx->state, p);
            p += 64;
            len -= 64;
        }
        idx = 0;
    }
    if (len > 0) memcpy(ctx->buffer + idx, p, len);
    return 1;
}

int EVP_DigestFinal_ex(EVP_MD_CTX* ctx, unsigned char* md, unsigned int* size)
{
    uint64_t bits;
    size_t idx;
    uint8_t* b;
    int i;

    if (!ctx || !md) return 0;

    bits = ctx->count * 8u;
    idx = (size_t)(ctx->count % 64);
    b = ctx->buffer;

    b[idx++] = 0x80;
    if (idx > 56)
    {
        memset(b + idx, 0, 64 - idx);
        md5_transform(ctx->state, b);
        idx = 0;
    }
    memset(b + idx, 0, 56 - idx);
    for (i = 0; i < 8; i++) b[56 + i] = (uint8_t)((bits >> (8 * i)) & 0xFF);
    md5_transform(ctx->state, b);

    for (i = 0; i < 4; i++)
    {
        md[i * 4 + 0] = (uint8_t)(ctx->state[i] & 0xFF);
        md[i * 4 + 1] = (uint8_t)((ctx->state[i] >> 8) & 0xFF);
        md[i * 4 + 2] = (uint8_t)((ctx->state[i] >> 16) & 0xFF);
        md[i * 4 + 3] = (uint8_t)((ctx->state[i] >> 24) & 0xFF);
    }
    if (size) *size = 16;
    return 1;
}

void simssl_md5(const unsigned char* data, size_t len, unsigned char out[16])
{
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    unsigned int n = 0;
    if (!ctx) return;
    EVP_DigestInit_ex(ctx, EVP_md5(), NULL);
    EVP_DigestUpdate(ctx, data, len);
    EVP_DigestFinal_ex(ctx, out, &n);
    EVP_MD_CTX_free(ctx);
}

/* ---------------------------- 对称密码 ---------------------------- */

const EVP_CIPHER* EVP_aes_128_ecb(void) { return &g_aes_128_ecb; }
const EVP_CIPHER* EVP_aes_128_cbc(void) { return &g_aes_128_cbc; }
const EVP_CIPHER* EVP_des_ede3_ecb(void) { return &g_des_ede3_ecb; }
const EVP_CIPHER* EVP_des_ede3_cbc(void) { return &g_des_ede3_cbc; }

EVP_CIPHER_CTX* EVP_CIPHER_CTX_new(void)
{
    EVP_CIPHER_CTX* ctx = (EVP_CIPHER_CTX*)malloc(sizeof(EVP_CIPHER_CTX));
    if (ctx) memset(ctx, 0, sizeof(*ctx));
    return ctx;
}

void EVP_CIPHER_CTX_free(EVP_CIPHER_CTX* ctx)
{
    if (ctx) free(ctx);
}

int EVP_CIPHER_CTX_set_padding(EVP_CIPHER_CTX* ctx, int padding)
{
    if (!ctx) return 0;
    ctx->padding = padding ? 1 : 0;
    return 1;
}

static int simssl_cipher_init(EVP_CIPHER_CTX* ctx, const EVP_CIPHER* type,
                             const unsigned char* key, const unsigned char* iv, int enc)
{
    if (!ctx || !type) return 0;

    ctx->cipher = type;
    ctx->enc = enc;
    ctx->buflen = 0;
    ctx->have_hold = 0;
    /* 与 OpenSSL 相同：默认开启 padding，需显式 set_padding(ctx, 0) 关闭 */
    ctx->padding = 1;

    if (key)
    {
        memcpy(ctx->key, key, (size_t)type->key_len);
        if (type->alg == MYSSL_ALG_AES_128_ECB || type->alg == MYSSL_ALG_AES_128_CBC)
        {
            aes128_expand_key(ctx->key, ctx->rk);
        }
        else
        {
            des_key_schedule(ctx->key, ctx->dk[0]);
            des_key_schedule(ctx->key + 8, ctx->dk[1]);
            des_key_schedule(ctx->key + 16, ctx->dk[2]);
        }
    }

    if (type->iv_len > 0 && iv) memcpy(ctx->iv, iv, (size_t)type->iv_len);
    memset(ctx->chain, 0, sizeof(ctx->chain));
    if (type->iv_len > 0) memcpy(ctx->chain, ctx->iv, (size_t)type->iv_len);
    return 1;
}

int EVP_EncryptInit_ex(EVP_CIPHER_CTX* ctx, const EVP_CIPHER* type, void* impl,
                       const unsigned char* key, const unsigned char* iv)
{
    (void)impl;
    return simssl_cipher_init(ctx, type, key, iv, 1);
}

int EVP_DecryptInit_ex(EVP_CIPHER_CTX* ctx, const EVP_CIPHER* type, void* impl,
                       const unsigned char* key, const unsigned char* iv)
{
    (void)impl;
    return simssl_cipher_init(ctx, type, key, iv, 0);
}

/* 单块处理，含 ECB/CBC 模式与链值维护 */
static void simssl_crypt_block(EVP_CIPHER_CTX* ctx, const uint8_t* in, uint8_t* out)
{
    const int bs = ctx->cipher->block_size;
    int is_des = (ctx->cipher->alg == MYSSL_ALG_DES_EDE3_ECB ||
                  ctx->cipher->alg == MYSSL_ALG_DES_EDE3_CBC);
    int is_cbc = (ctx->cipher->alg == MYSSL_ALG_AES_128_CBC ||
                  ctx->cipher->alg == MYSSL_ALG_DES_EDE3_CBC);
    uint8_t tmp[16];
    int i;

    if (is_cbc)
    {
        if (ctx->enc)
        {
            /* CBC 加密：C = E(P xor chain)，链值更新为密文块 */
            for (i = 0; i < bs; i++) tmp[i] = (uint8_t)(in[i] ^ ctx->chain[i]);
            if (is_des) simssl_des3_encrypt_block(ctx->key, tmp, out);
            else aes128_encrypt_block_rk(ctx->rk, tmp, out);
            memcpy(ctx->chain, out, (size_t)bs);
        }
        else
        {
            /* CBC 解密：P = D(C) xor chain，链值更新为密文块 */
            if (is_des) simssl_des3_decrypt_block(ctx->key, in, tmp);
            else aes128_decrypt_block_rk(ctx->rk, in, tmp);
            for (i = 0; i < bs; i++) out[i] = (uint8_t)(tmp[i] ^ ctx->chain[i]);
            memcpy(ctx->chain, in, (size_t)bs);
        }
        return;
    }

    if (is_des)
    {
        if (ctx->enc) simssl_des3_encrypt_block(ctx->key, in, out);
        else simssl_des3_decrypt_block(ctx->key, in, out);
    }
    else
    {
        if (ctx->enc) aes128_encrypt_block_rk(ctx->rk, in, out);
        else aes128_decrypt_block_rk(ctx->rk, in, out);
    }
}

static int simssl_update(EVP_CIPHER_CTX* ctx, unsigned char* out, int* outl,
                        const unsigned char* in, int inl)
{
    const int bs = ctx->cipher->block_size;
    int produced = 0;
    const uint8_t* p = in;
    int left = inl;

    if (!ctx || !outl || inl < 0) return 0;
    if (inl > 0 && !in) return 0;
    *outl = 0;

    while (left > 0)
    {
        size_t take = (size_t)bs - ctx->buflen;
        if (take > (size_t)left) take = (size_t)left;
        memcpy(ctx->buf + ctx->buflen, p, take);
        ctx->buflen += take;
        p += take;
        left -= (int)take;

        if (ctx->buflen == (size_t)bs)
        {
            if (!ctx->enc && ctx->padding)
            {
                /* 解密且开启 padding：延迟一块，供 Final 去填充 */
                if (ctx->have_hold)
                {
                    memcpy(out + produced, ctx->hold, (size_t)bs);
                    produced += bs;
                }
                simssl_crypt_block(ctx, ctx->buf, ctx->hold);
                ctx->have_hold = 1;
            }
            else
            {
                simssl_crypt_block(ctx, ctx->buf, out + produced);
                produced += bs;
            }
            ctx->buflen = 0;
        }
    }

    *outl = produced;
    return 1;
}

int EVP_EncryptUpdate(EVP_CIPHER_CTX* ctx, unsigned char* out, int* outl,
                      const unsigned char* in, int inl)
{
    if (!ctx || !ctx->cipher) return 0;
    return simssl_update(ctx, out, outl, in, inl);
}

int EVP_DecryptUpdate(EVP_CIPHER_CTX* ctx, unsigned char* out, int* outl,
                      const unsigned char* in, int inl)
{
    if (!ctx || !ctx->cipher) return 0;
    return simssl_update(ctx, out, outl, in, inl);
}

int EVP_EncryptFinal_ex(EVP_CIPHER_CTX* ctx, unsigned char* out, int* outl)
{
    const int bs = ctx->cipher ? ctx->cipher->block_size : 0;
    if (!ctx || !ctx->cipher || !outl) return 0;
    *outl = 0;

    if (!ctx->padding)
    {
        /* 关闭 padding 时，残留不足一块的数据视为错误（与 OpenSSL 一致） */
        return ctx->buflen == 0 ? 1 : 0;
    }

    {
        uint8_t pad = (uint8_t)(bs - (int)ctx->buflen);
        memset(ctx->buf + ctx->buflen, pad, pad);
        ctx->buflen = 0;
        simssl_crypt_block(ctx, ctx->buf, out);
        *outl = bs;
    }
    return 1;
}

int EVP_DecryptFinal_ex(EVP_CIPHER_CTX* ctx, unsigned char* out, int* outl)
{
    const int bs = ctx->cipher ? ctx->cipher->block_size : 0;
    if (!ctx || !ctx->cipher || !outl) return 0;
    *outl = 0;

    if (!ctx->padding)
    {
        return ctx->buflen == 0 ? 1 : 0;
    }

    if (ctx->buflen != 0 || !ctx->have_hold) return 0;

    {
        uint8_t pad = ctx->hold[bs - 1];
        int i;
        if (pad == 0 || pad > bs) return 0;
        for (i = bs - pad; i < bs; i++)
        {
            if (ctx->hold[i] != pad) return 0;
        }
        memcpy(out, ctx->hold, (size_t)(bs - pad));
        *outl = bs - pad;
        ctx->have_hold = 0;
    }
    return 1;
}
