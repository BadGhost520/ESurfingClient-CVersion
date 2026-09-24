#include "utils/sim/SimEvp.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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

struct myssl_md_st
{
    int id;
};

struct myssl_md_ctx_st
{
    uint32_t state[4];
    uint64_t count;
    uint8_t buffer[64];
};

static const EVP_MD g_md5 = {1};

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
