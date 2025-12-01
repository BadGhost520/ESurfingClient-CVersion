#include "../../headFiles/cipher/CipherInterface.h"
#include "../../headFiles/cipher/CipherUtils.h"
#include <stdint.h>
#include <string.h>

typedef struct {
    uint32_t k0[4];
    uint32_t k1[4];
    uint32_t k2[4];
    uint32_t iv[2];
} ab6c8_ctx_t;

static uint32_t bswap32(const uint32_t x)
{
    return (x & 0x000000FFU) << 24 |
           (x & 0x0000FF00U) << 8  |
           (x & 0x00FF0000U) >> 8  |
           (x & 0xFF000000U) >> 24;
}

static uint32_t get_uint32_le(const uint8_t* data, const size_t offset)
{
    return (uint32_t)data[offset] |
           (uint32_t)data[offset + 1] << 8 |
           (uint32_t)data[offset + 2] << 16 |
           (uint32_t)data[offset + 3] << 24;
}

static void set_uint32_le(uint8_t* data, const size_t offset, const uint32_t v)
{
    data[offset]     = (uint8_t)(v & 0xFF);
    data[offset + 1] = (uint8_t)(v >> 8 & 0xFF);
    data[offset + 2] = (uint8_t)(v >> 16 & 0xFF);
    data[offset + 3] = (uint8_t)(v >> 24 & 0xFF);
}

static void ab6c8_block_decrypt(uint32_t* v0_le, uint32_t* v1_le, const uint32_t* k_le)
{
    uint32_t v17[4];
    v17[0] = bswap32(k_le[0]);
    v17[1] = bswap32(k_le[1]);
    v17[2] = bswap32(k_le[2]);
    v17[3] = bswap32(k_le[3]);
    uint32_t v9  = bswap32(*v0_le);
    uint32_t v10 = bswap32(*v1_le);
    const uint32_t delta = 1640531527U;
    uint32_t i = delta * (uint32_t)-32;
    while (i)
    {
        const uint32_t v14 = v10 - (16U * v9 ^ v9 >> 5) - (v9 ^ i);
        const uint32_t v15 = i;
        i += delta;
        v10 = v14 - v17[v15 >> 11 & 3U];
        v9 -= (16U * v10 ^ v10 >> 5) + (i ^ v10) + v17[i & 3U];
    }
    *v0_le = bswap32(v9);
    *v1_le = bswap32(v10);
}

static void ab6c8_block_encrypt(uint32_t* v0_le, uint32_t* v1_le, const uint32_t* k_le)
{
    uint32_t v17[4];
    v17[0] = bswap32(k_le[0]);
    v17[1] = bswap32(k_le[1]);
    v17[2] = bswap32(k_le[2]);
    v17[3] = bswap32(k_le[3]);
    uint32_t v9  = bswap32(*v0_le);
    uint32_t v10 = bswap32(*v1_le);
    const uint32_t delta = 1640531527U;
    uint32_t j = 0;
    uint32_t v6 = v17[0];
    uint32_t v8 = v17[3];
    const uint32_t v11 = 0u - delta * 32u;
    while (1)
    {
        v9  += (16U * v10 ^ v10 >> 5) + (j ^ v10) + v6;
        v10 += (16U * v9 ^ v9 >> 5) + ((j - delta) ^ v9) + v8;
        if (v11 == j - delta) break;
        v6 = v17[(uint8_t)(j - 71U) & 3U];
        v8 = v17[(j + 1013904242U) >> 11 & 3U];
        j  -= delta;
    }
    *v0_le = bswap32(v9);
    *v1_le = bswap32(v10);
}

static char* ab6c8_encrypt(cipherInterfaceT* self, const char* text)
{
    if (!self || !text) return NULL;
    ab6c8_ctx_t* ctx = self->private_data;
    if (!ctx) return NULL;
    const size_t len = safeStrLen(text);
    size_t padded_len = 0;
    uint8_t* padded = padToMultiple((const uint8_t*)text, len, 8, &padded_len);
    if (!padded) return NULL;
    uint8_t* out = safeMalloc(padded_len);
    memcpy(out, padded, padded_len);
    safeFree(padded);
    uint32_t chain0 = ctx->iv[0];
    uint32_t chain1 = ctx->iv[1];
    for (size_t i = 0; i < padded_len; i += 8)
    {
        uint32_t v0 = get_uint32_le(out, i);
        uint32_t v1 = get_uint32_le(out, i + 4);
        v0 ^= chain0; v1 ^= chain1;
        ab6c8_block_encrypt(&v0, &v1, ctx->k2);
        ab6c8_block_encrypt(&v0, &v1, ctx->k1);
        ab6c8_block_encrypt(&v0, &v1, ctx->k0);
        set_uint32_le(out, i, v0);
        set_uint32_le(out, i + 4, v1);
        chain0 = v0; chain1 = v1;
    }
    char* hex = bytesToHexUpper(out, padded_len);
    safeFree(out);
    return hex;
}

static char* ab6c8_decrypt(cipherInterfaceT* self, const char* hex)
{
    if (!self || !hex) return NULL;
    ab6c8_ctx_t* ctx = self->private_data;
    if (!ctx) return NULL;
    size_t ct_len = 0;
    uint8_t* ct = hexToBytes(hex, &ct_len);
    if (!ct) return NULL;
    uint8_t* out = safeMalloc(ct_len);
    memcpy(out, ct, ct_len);
    uint32_t prev0 = ctx->iv[0];
    uint32_t prev1 = ctx->iv[1];
    for (size_t i = 0; i < ct_len; i += 8)
    {
        uint32_t v0 = get_uint32_le(out, i);
        uint32_t v1 = get_uint32_le(out, i + 4);
        const uint32_t c0 = v0, c1 = v1;
        ab6c8_block_decrypt(&v0, &v1, ctx->k0);
        ab6c8_block_decrypt(&v0, &v1, ctx->k1);
        ab6c8_block_decrypt(&v0, &v1, ctx->k2);
        v0 ^= prev0; v1 ^= prev1;
        set_uint32_le(out, i, v0);
        set_uint32_le(out, i + 4, v1);
        prev0 = c0; prev1 = c1;
    }

    safeFree(ct);
    while (ct_len > 0 && out[ct_len - 1] == 0) ct_len--;
    char* text = safeMalloc(ct_len + 1);
    memcpy(text, out, ct_len);
    text[ct_len] = '\0';
    safeFree(out);
    return text;
}

static void ab6c8_destroy(cipherInterfaceT* self)
{
    if (!self) return;
    if (self->private_data) safeFree(self->private_data);
    safeFree(self);
}

cipherInterfaceT* create_ab6c8_cipher(const uint32_t* key0, const uint32_t* key1,
                                        const uint32_t* key2, const uint32_t* iv)
{
    if (!key0 || !key1 || !key2 || !iv) return NULL;
    cipherInterfaceT* ci = safeCalloc(1, sizeof(cipherInterfaceT));
    ab6c8_ctx_t* ctx = safeCalloc(1, sizeof(ab6c8_ctx_t));
    memcpy(ctx->k0, key0, 4 * sizeof(uint32_t));
    memcpy(ctx->k1, key1, 4 * sizeof(uint32_t));
    memcpy(ctx->k2, key2, 4 * sizeof(uint32_t));
    memcpy(ctx->iv, iv, 2 * sizeof(uint32_t));
    ci->encrypt = ab6c8_encrypt;
    ci->decrypt = ab6c8_decrypt;
    ci->destroy = ab6c8_destroy;
    ci->private_data = ctx;
    return ci;
}