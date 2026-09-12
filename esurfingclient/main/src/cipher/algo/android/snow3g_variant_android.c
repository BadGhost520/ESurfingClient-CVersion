#include "cipher/CipherInterface.h"
#include "cipher/CipherUtils.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define SNOW3G_CELLS 22
#define SNOW3G_KEY_SIZE 16
#define SNOW3G_IV_SIZE 16

typedef struct {
    uint8_t key[SNOW3G_KEY_SIZE];
    uint8_t iv[SNOW3G_IV_SIZE];
} snow3g_variant_android_data_t;

static const uint32_t SNOW3G_D[16] = {
    0x000044D7,0x000026BC,0x0000626B,0x0000135E,0x00005789,0x000035E2,0x00007135,0x000009AF,
    0x00004D78,0x00002F13,0x00006BC4,0x00001AF1,0x00005E26,0x00003C4D,0x0000789A,0x000047AC
};

static const uint8_t SNOW3G_S0[256] = {
    0x3e,0x72,0x5b,0x47,0xca,0xe0,0x00,0x33,0x04,0xd1,0x54,0x98,0x09,0xb9,0x6d,0xcb,0x7b,0x1b,0xf9,0x32,0xaf,0x9d,0x6a,0xa5,0xb8,0x2d,0xfc,0x1d,0x08,0x53,0x03,0x90,0x4d,0x4e,0x84,0x99,0xe4,0xce,0xd9,0x91,0xdd,0xb6,0x85,0x48,0x8b,0x29,0x6e,0xac,0xcd,0xc1,0xf8,0x1e,0x73,0x43,0x69,0xc6,0xb5,0xbd,0xfd,0x39,0x63,0x20,0xd4,0x38,0x76,0x7d,0xb2,0xa7,0xcf,0xed,0x57,0xc5,0xf3,0x2c,0xbb,0x14,0x21,0x06,0x55,0x9b,0xe3,0xef,0x5e,0x31,0x4f,0x7f,0x5a,0xa4,0x0d,0x82,0x51,0x49,0x5f,0xba,0x58,0x1c,0x4a,0x16,0xd5,0x17,0xa8,0x92,0x24,0x1f,0x8c,0xff,0xd8,0xae,0x2e,0x01,0xd3,0xad,0x3b,0x4b,0xda,0x46,0xeb,0xc9,0xde,0x9a,0x8f,0x87,0xd7,0x3a,0x80,0x6f,0x2f,0xc8,0xb1,0xb4,0x37,0xf7,0x0a,0x22,0x13,0x28,0x7c,0xcc,0x3c,0x89,0xc7,0xc3,0x96,0x56,0x07,0xbf,0x7e,0xf0,0x0b,0x2b,0x97,0x52,0x35,0x41,0x79,0x61,0xa6,0x4c,0x10,0xfe,0xbc,0x26,0x95,0x88,0x8a,0xb0,0xa3,0xfb,0xc0,0x18,0x94,0xf2,0xe1,0xe5,0xe9,0x5d,0xd0,0xdc,0x11,0x66,0x64,0x5c,0xec,0x59,0x42,0x75,0x12,0xf5,0x74,0x9c,0xaa,0x23,0x0e,0x86,0xab,0xbe,0x2a,0x02,0xe7,0x67,0xe6,0x44,0xa2,0x6c,0xc2,0x93,0x9f,0xf1,0xf6,0xfa,0x36,0xd2,0x50,0x68,0x9e,0x62,0x71,0x15,0x3d,0xd6,0x40,0xc4,0xe2,0x0f,0x8e,0x83,0x77,0x6b,0x25,0x05,0x3f,0x0c,0x30,0xea,0x70,0xb7,0xa1,0xe8,0xa9,0x65,0x8d,0x27,0x1a,0xdb,0x81,0xb3,0xa0,0xf4,0x45,0x7a,0x19,0xdf,0xee,0x78,0x34,0x60
};

static const uint8_t SNOW3G_S1[256] = {
    0x55,0xc2,0x63,0x71,0x3b,0xc8,0x47,0x86,0x9f,0x3c,0xda,0x5b,0x29,0xaa,0xfd,0x77,0x8c,0xc5,0x94,0x0c,0xa6,0x1a,0x13,0x00,0xe3,0xa8,0x16,0x72,0x40,0xf9,0xf8,0x42,0x44,0x26,0x68,0x96,0x81,0xd9,0x45,0x3e,0x10,0x76,0xc6,0xa7,0x8b,0x39,0x43,0xe1,0x3a,0xb5,0x56,0x2a,0xc0,0x6d,0xb3,0x05,0x22,0x66,0xbf,0xdc,0x0b,0xfa,0x62,0x48,0xdd,0x20,0x11,0x06,0x36,0xc9,0xc1,0xcf,0xf6,0x27,0x52,0xbb,0x69,0xf5,0xd4,0x87,0x7f,0x84,0x4c,0xd2,0x9c,0x57,0xa4,0xbc,0x4f,0x9a,0xdf,0xfe,0xd6,0x8d,0x7a,0xeb,0x2b,0x53,0xd8,0x5c,0xa1,0x14,0x17,0xfb,0x23,0xd5,0x7d,0x30,0x67,0x73,0x08,0x09,0xee,0xb7,0x70,0x3f,0x61,0xb2,0x19,0x8e,0x4e,0xe5,0x4b,0x93,0x8f,0x5d,0xdb,0xa9,0xad,0xf1,0xae,0x2e,0xcb,0x0d,0xfc,0xf4,0x2d,0x46,0x6e,0x1d,0x97,0xe8,0xd1,0xe9,0x4d,0x37,0xa5,0x75,0x5e,0x83,0x9e,0xab,0x82,0x9d,0xb9,0x1c,0xe0,0xcd,0x49,0x89,0x01,0xb6,0xbd,0x58,0x24,0xa2,0x5f,0x38,0x78,0x99,0x15,0x90,0x50,0xb8,0x95,0xe4,0xd0,0x91,0xc7,0xce,0xed,0x0f,0xb4,0x6f,0xa0,0xcc,0xf0,0x02,0x4a,0x79,0xc3,0xde,0xa3,0xef,0xea,0x51,0xe6,0x6b,0x18,0xec,0x1b,0x2c,0x80,0xf7,0x74,0xe7,0xff,0x21,0x5a,0x6a,0x54,0x1e,0x41,0x31,0x92,0x35,0xc4,0x33,0x07,0x0a,0xba,0x7e,0x0e,0x34,0x88,0xb1,0x98,0x7c,0xf3,0x3d,0x60,0x6c,0x7b,0xca,0xd3,0x1f,0x32,0x65,0x04,0x28,0x64,0xbe,0x85,0x9b,0x2f,0x59,0x8a,0xd7,0xb0,0x25,0xac,0xaf,0x12,0x03,0xe2,0xf2
};

static uint32_t snow3g_bswap32(uint32_t x)
{
    return ((x & 0xFF) << 24) | ((x & 0xFF00) << 8) | ((x >> 8) & 0xFF00) | (x >> 24);
}

static uint32_t snow3g_sbox(uint32_t x)
{
    return ((uint32_t)SNOW3G_S0[x >> 24] << 24) | ((uint32_t)SNOW3G_S1[(x >> 16) & 0xFF] << 16)
         | ((uint32_t)SNOW3G_S0[(x >> 8) & 0xFF] << 8) | (uint32_t)SNOW3G_S1[x & 0xFF];
}

static uint32_t snow3g_fsm(uint32_t *c)
{
    const uint32_t v1 = c[1];
    const uint32_t v2 = c[3] + c[0];
    const uint32_t v3 = c[4] ^ v1;
    const uint32_t v4 = (c[0] ^ c[2]) + v1;
    const uint32_t v5 = (v2 << 16) | (v3 >> 16);
    const uint32_t v6 = v2 >> 14;
    const uint32_t v7 = v2 >> 6;
    const uint32_t v8 = v2 >> 16;
    const uint32_t v9 = (v3 << 16) | (v2 >> 16);
    const uint32_t t1 = (uint32_t)((((uint64_t)(v3 >> 16)) << 32 | v5) >> 14);
    const uint32_t t2 = (uint32_t)((((uint64_t)(v3 >> 16)) << 32 | v5) >> 8);
    const uint32_t v10 = ((v6 & 3) | (4 * v5)) ^ v5 ^ ((v7 & 0x3FF) | (v5 << 10)) ^ t1 ^ t2;
    const uint32_t t3 = (uint32_t)((((uint64_t)v8 << 32) | v9) >> 10);
    const uint32_t t4 = (uint32_t)((((uint64_t)v8 << 32) | v9) >> 2);
    const uint32_t v11 = (((v3 >> 8) & 0xFF) | (v9 << 8)) ^ v9 ^ (((v3 & 0xFFFF) >> 2) | (v9 << 14)) ^ t3 ^ t4;
    c[0] = snow3g_sbox(v10);
    c[1] = snow3g_sbox(v11);
    return v4;
}

static void snow3g_keyschedule(uint32_t *c, const uint8_t *key, const uint8_t *iv)
{
    memset(c, 0, SNOW3G_CELLS * sizeof(uint32_t));
    for (int i = 0; i < 16; ++i)
        c[i + 6] = (iv[i] & 0x807FFFFF) | ((uint32_t)key[i] << 23) | ((SNOW3G_D[i] & 0x7FFFFF) << 8);

    uint32_t v9  = c[21];
    uint32_t v10 = c[20] & 0xFFFF;
    uint32_t v11 = c[17];
    uint32_t v12 = c[15];
    uint32_t v13 = c[13];
    uint32_t v14 = c[11];
    uint32_t v15 = c[8];
    uint32_t v16 = c[6];
    int rounds = 32;
    do {
        c[2] = (uint16_t)v10 | ((uint16_t)(v9 >> 15) << 16);
        c[3] = (v12 >> 15) | (v11 << 16);
        c[4] = (v14 >> 15) | (v13 << 16);
        c[5] = (v16 >> 15) | (v15 << 16);
        const uint32_t v18 = snow3g_fsm(c);
        const uint64_t v19_hi = ((uint64_t)c[13] << 32) | c[12];
        const uint64_t v20    = ((uint64_t)c[20] << 32) | c[19];
        const uint32_t v15n   = c[9];
        const uint32_t v16n   = c[7];
        c[7] = c[8];
        const uint64_t v22_lo = ((uint64_t)c[15] << 32) | c[14];
        const uint64_t v22_hi = ((uint64_t)c[17] << 32) | c[16];
        const uint32_t v23 = (((c[6] & 0x7FFFFF) << 8) | (c[6] >> 23)) + c[6];
        const uint32_t v24 = (((c[10] & 0x7FF) << 20) | (c[10] >> 11))
                             + (v23 >> 31) + (v23 & 0x7FFFFFFF);
        const uint32_t v25 = (((c[16] & 0x3FF) << 21) | (c[16] >> 10))
                             + (v24 >> 31) + (v24 & 0x7FFFFFFF);
        const uint32_t v10n = c[21];
        const uint32_t v12n = (uint32_t)(v22_hi & 0xFFFFFFFF);
        const uint32_t v26 = (((c[19] & 0x3FFF) << 17) | (c[19] >> 14))
                             + (v25 >> 31) + (v25 & 0x7FFFFFFF);
        const uint32_t v27 = (((v10n & 0xFFFF) << 15) | (v10n >> 16))
                             + (v26 >> 31) + (v26 & 0x7FFFFFFF);
        const uint32_t v11n = c[18];
        const uint32_t v28 = (v27 >> 31) + (v18 >> 1) + (v27 & 0x7FFFFFFF);
        const uint32_t v14n = (uint32_t)(v19_hi & 0xFFFFFFFF);
        const uint32_t v13n = (uint32_t)(v22_lo & 0xFFFFFFFF);
        const uint32_t v9n  = (v28 & 0x7FFFFFFF) + (v28 >> 31);
        c[9] = c[10]; c[10] = c[11]; c[11] = c[12];
        c[12] = (uint32_t)(v19_hi >> 32);
        c[18] = c[19];
        c[19] = (uint32_t)(v20 >> 32);
        c[6] = v16n;
        c[8] = v15n;
        c[13] = (uint32_t)(v22_lo & 0xFFFFFFFF);
        c[14] = (uint32_t)(v22_lo >> 32);
        c[15] = (uint32_t)(v22_hi & 0xFFFFFFFF);
        c[16] = (uint32_t)(v22_hi >> 32);
        c[17] = v11n;
        c[20] = v10n;
        c[21] = v9n;
        v9 = v9n; v10 = v10n;
        v11 = v11n; v12 = v12n; v13 = v13n; v14 = v14n; v15 = v15n; v16 = v16n;
    } while (--rounds);
}

static void snow3g_generate(uint32_t *c, const uint8_t *in, const int blocks, uint8_t *out)
{
    const uint32_t v4 = (c[15] >> 15) | (c[17] << 16);
    const uint32_t v5 = (c[11] >> 15) | (c[13] << 16);
    const uint32_t v6 = (c[6] >> 15) | (c[8] << 16);
    c[2] = (uint16_t)c[20] | ((uint16_t)(c[21] >> 15) << 16);
    c[3] = v4;
    c[4] = v5;
    c[5] = v6;
    snow3g_fsm(c);
    for (int i = 0;; ++i)
    {
        const uint64_t r_lo = ((uint64_t)c[11] << 32) | c[10];
        const uint64_t r_hi = ((uint64_t)c[13] << 32) | c[12];
        const uint32_t v13 = c[7];
        const uint32_t v14 = c[18];
        const uint32_t v15 = c[19];
        const uint32_t v17 = c[20];
        const uint32_t v16 = c[21];
        const uint32_t v18 = c[6];
        c[7] = c[8];
        c[18] = v15;
        c[19] = v17;
        const uint64_t v19_lo = ((uint64_t)c[15] << 32) | c[14];
        const uint64_t v19_hi = ((uint64_t)c[17] << 32) | c[16];
        const uint32_t v20 = (((v18 & 0x7FFFFF) << 8) | (v18 >> 23)) + v18;
        const uint32_t v21 = (((r_lo & 0x7FF) << 20) | ((uint32_t)r_lo >> 11))
                             + (v20 >> 31) + (v20 & 0x7FFFFFFF);
        const uint32_t v22 = (((c[16] & 0x3FF) << 21) | (c[16] >> 10))
                             + (v21 >> 31) + (v21 & 0x7FFFFFFF);
        const uint32_t v23 = (((v15 & 0x3FFF) << 17) | (v15 >> 14)) + (v22 >> 31) + (v22 & 0x7FFFFFFF);
        const uint32_t v24 = c[9];
        const uint32_t v25 = (((v16 & 0xFFFF) << 15) | (v16 >> 16)) + (v23 >> 31) + (v23 & 0x7FFFFFFF);
        const uint32_t v26 = (v25 & 0x7FFFFFFF) + (v25 >> 31);
        const uint32_t old_c16 = c[16];
        c[6] = v13;
        c[8] = v24;
        c[9] = c[10]; c[10] = c[11]; c[11] = c[12];
        c[12] = (uint32_t)(r_hi >> 32);
        c[13] = (uint32_t)(v19_lo & 0xFFFFFFFF);
        c[14] = (uint32_t)(v19_lo >> 32);
        c[15] = (uint32_t)(v19_hi & 0xFFFFFFFF);
        c[16] = (uint32_t)(v19_hi >> 32);
        c[17] = v14;
        c[20] = v16;
        c[21] = v26;
        if (i >= blocks)
            break;
        c[2] = (uint16_t)v16 | ((uint16_t)(v26 >> 15) << 16);
        c[3] = (v14 << 16) | (old_c16 >> 15);
        c[4] = ((uint32_t)v19_lo << 16) | ((uint32_t)(r_hi & 0xFFFFFFFF) >> 15);
        c[5] = (v24 << 16) | (v13 >> 15);
        const uint32_t ks = snow3g_fsm(c) ^ c[5];
        const uint32_t pt = snow3g_bswap32(((const uint32_t *)in)[i]);
        ((uint32_t *)out)[i] = snow3g_bswap32(pt ^ ks);
    }
}

static uint8_t* snow3g_encrypt_raw(const uint8_t* key, const uint8_t* iv, const uint8_t* data,
                                   const size_t data_len, size_t* output_len)
{
    const int pad = (data_len & 3) ? 4 - (int)(data_len & 3) : 0;
    const int total = pad + (int)data_len;
    uint8_t* buf = s_calloc(1, total ? (size_t)total : 1);
    if(!buf) return NULL;
    memcpy(buf, data, data_len);
    uint32_t c[SNOW3G_CELLS];
    snow3g_keyschedule(c, key, iv);
    snow3g_generate(c, buf, total / 4, buf);
    *output_len = (size_t)total;
    return buf;
}

static uint8_t* snow3g_decrypt_raw(const uint8_t* key, const uint8_t* iv, const uint8_t* data,
                                   const size_t data_len, size_t* output_len)
{
    uint8_t* buf = s_malloc(data_len ? data_len : 1);
    if(!buf) return NULL;
    memcpy(buf, data, data_len);
    uint32_t c[SNOW3G_CELLS];
    snow3g_keyschedule(c, key, iv);
    snow3g_generate(c, buf, (int)data_len / 4, buf);
    *output_len = data_len;
    return buf;
}

static char* snow3g_variant_encrypt(cipher_interface_t* self, const char* text)
{
    if(!self || !text) return NULL;
    const snow3g_variant_android_data_t* d = self->private_data;
    const size_t text_len = strlen(text);
    size_t out_len=0; uint8_t* out = snow3g_encrypt_raw(d->key, d->iv, (const uint8_t*)text, text_len, &out_len);
    if(!out) return NULL;
    char* hex = bytes_2_hex(out, out_len);
    s_free(out);
    return hex;
}

static char* snow3g_variant_decrypt(cipher_interface_t* self, const char* hex)
{
    if(!self || !hex) return NULL;
    const snow3g_variant_android_data_t* d = self->private_data;
    size_t bytes_len=0; uint8_t* bytes = hex_2_bytes(hex, &bytes_len);
    if(!bytes) return NULL;
    size_t out_len=0; uint8_t* out = snow3g_decrypt_raw(d->key, d->iv, bytes, bytes_len, &out_len);
    s_free(bytes);
    if(!out) return NULL;
    while(out_len > 0 && out[out_len - 1] == 0) out_len--;
    char* result = s_malloc(out_len + 1);
    memcpy(result, out, out_len);
    result[out_len] = '\0';
    s_free(out);
    return result;
}

static void snow3g_variant_destroy(cipher_interface_t* self)
{
    if(self)
    {
        s_free(self->private_data);
        s_free(self);
    }
}

cipher_interface_t* create_snow3g_variant_android_cipher(const uint8_t* key, const uint8_t* iv)
{
    if(!key || !iv) return NULL;
    cipher_interface_t* ci = s_calloc(1, sizeof(cipher_interface_t));
    snow3g_variant_android_data_t* d = s_malloc(sizeof(snow3g_variant_android_data_t));
    memcpy(d->key, key, SNOW3G_KEY_SIZE);
    memcpy(d->iv, iv, SNOW3G_IV_SIZE);
    ci->encrypt = snow3g_variant_encrypt;
    ci->decrypt = snow3g_variant_decrypt;
    ci->destroy = snow3g_variant_destroy;
    ci->private_data = d;
    return ci;
}
