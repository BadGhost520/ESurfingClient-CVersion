#include "cipher/CipherInterface.h"
#include "cipher/CipherUtils.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------
 * Algo Id: BB2EA626-590B-4C42-82BE-E052FCBBB88E / DEABB8C8-A2BC-48CA-8ED0-8CDF1BD62F61
 *          (Double-layer AES-128-CBC, Android)
 * Two AES-128-CBC layers (32-byte key = two 16-byte layers), custom
 * key expansion (transposed key + 10 rounds of S-box/Rcon mixing ->
 * 176-byte schedule), standard AES round structure, row-major state,
 * IV-prefix mode with PKCS7 padding.
 * Translated from IDA pseudocode (sub_1FA0 keysched / sub_27F0 block /
 * sub_3444 double-layer enc / sub_2ED8 layer dec / sub_34D8 double dec).
 * Verified against the real .so via the unicorn emulator.
 * ------------------------------------------------------------------ */

#define AES_DOUBLE_KEY_SIZE 32
#define AES_DOUBLE_BLOCK_SIZE 16

typedef struct {
    uint8_t key[AES_DOUBLE_KEY_SIZE];
    uint8_t iv[AES_DOUBLE_BLOCK_SIZE];
} aes_double_cbc_android_data_t;

static const uint8_t AES_DOUBLE_S[256] = {
    0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
    0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
    0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
    0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
    0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
    0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
    0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
    0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
    0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
    0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
    0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
    0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
    0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
    0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
    0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
    0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16
};
static const uint8_t AES_DOUBLE_INV_S[256] = {
    0x52,0x09,0x6a,0xd5,0x30,0x36,0xa5,0x38,0xbf,0x40,0xa3,0x9e,0x81,0xf3,0xd7,0xfb,
    0x7c,0xe3,0x39,0x82,0x9b,0x2f,0xff,0x87,0x34,0x8e,0x43,0x44,0xc4,0xde,0xe9,0xcb,
    0x54,0x7b,0x94,0x32,0xa6,0xc2,0x23,0x3d,0xee,0x4c,0x95,0x0b,0x42,0xfa,0xc3,0x4e,
    0x08,0x2e,0xa1,0x66,0x28,0xd9,0x24,0xb2,0x76,0x5b,0xa2,0x49,0x6d,0x8b,0xd1,0x25,
    0x72,0xf8,0xf6,0x64,0x86,0x68,0x98,0x16,0xd4,0xa4,0x5c,0xcc,0x5d,0x65,0xb6,0x92,
    0x6c,0x70,0x48,0x50,0xfd,0xed,0xb9,0xda,0x5e,0x15,0x46,0x57,0xa7,0x8d,0x9d,0x84,
    0x90,0xd8,0xab,0x00,0x8c,0xbc,0xd3,0x0a,0xf7,0xe4,0x58,0x05,0xb8,0xb3,0x45,0x06,
    0xd0,0x2c,0x1e,0x8f,0xca,0x3f,0x0f,0x02,0xc1,0xaf,0xbd,0x03,0x01,0x13,0x8a,0x6b,
    0x3a,0x91,0x11,0x41,0x4f,0x67,0xdc,0xea,0x97,0xf2,0xcf,0xce,0xf0,0xb4,0xe6,0x73,
    0x96,0xac,0x74,0x22,0xe7,0xad,0x35,0x85,0xe2,0xf9,0x37,0xe8,0x1c,0x75,0xdf,0x6e,
    0x47,0xf1,0x1a,0x71,0x1d,0x29,0xc5,0x89,0x6f,0xb7,0x62,0x0e,0xaa,0x18,0xbe,0x1b,
    0xfc,0x56,0x3e,0x4b,0xc6,0xd2,0x79,0x20,0x9a,0xdb,0xc0,0xfe,0x78,0xcd,0x5a,0xf4,
    0x1f,0xdd,0xa8,0x33,0x88,0x07,0xc7,0x31,0xb1,0x12,0x10,0x59,0x27,0x80,0xec,0x5f,
    0x60,0x51,0x7f,0xa9,0x19,0xb5,0x4a,0x0d,0x2d,0xe5,0x7a,0x9f,0x93,0xc9,0x9c,0xef,
    0xa0,0xe0,0x3b,0x4d,0xae,0x2a,0xf5,0xb0,0xc8,0xeb,0xbb,0x3c,0x83,0x53,0x99,0x61,
    0x17,0x2b,0x04,0x7e,0xba,0x77,0xd6,0x26,0xe1,0x69,0x14,0x63,0x55,0x21,0x0c,0x7d
};
static const uint8_t AES_DOUBLE_RC[10] = {0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80,0x1B,0x36};

static uint8_t aes_double_xtime(uint8_t x) { return (uint8_t)((x << 1) ^ ((x & 0x80) ? 0x1B : 0)); }
static uint8_t aes_double_mul(uint8_t a, uint8_t b)
{
    uint8_t p = 0;
    for (int i = 0; i < 8; i++)
    {
        if (b & 1) p ^= a;
        const uint8_t hi = a & 0x80; a <<= 1; if (hi) a ^= 0x1B; b >>= 1;
    }
    return p;
}

/* sub_1FA0: custom key expansion -> 176-byte schedule */
static void aes_double_key_expand(const uint8_t key[16], uint8_t sk[176])
{
    sk[0] = key[0];  sk[1] = key[4];  sk[2] = key[8];  sk[3] = key[12];
    sk[4] = key[1];  sk[5] = key[5];  sk[6] = key[9];  sk[7] = key[13];
    sk[8] = key[2];  sk[9] = key[6];  sk[10] = key[10]; sk[11] = key[14];
    sk[12] = key[3]; sk[13] = key[7]; sk[14] = key[11]; sk[15] = key[15];
    uint8_t *v3 = sk + 15;
    for (int v2 = 0; v2 < 10; ++v2)
    {
        const uint8_t v4 = AES_DOUBLE_S[v3[-8]] ^ AES_DOUBLE_RC[v2];
        const uint8_t v5 = AES_DOUBLE_S[v3[-4]] ^ v3[-11];
        const uint8_t v6 = v3[-15];
        const uint8_t v7 = AES_DOUBLE_S[v3[-12]] ^ v3[-3];
        const uint8_t v8 = v3[-10];
        const uint8_t v9 = AES_DOUBLE_S[v3[0]];
        v3[5] = v5;
        const uint8_t v10 = v3[-7];
        const uint8_t v11 = v5 ^ v8;
        const uint8_t v12 = v3[-2];
        const uint8_t v13 = v4 ^ v6;
        const uint8_t v14 = v3[-6];
        v3[13] = v7;
        const uint8_t v15 = v7 ^ v12;
        const uint8_t v16 = v3[-9];
        const uint8_t v17 = v9 ^ v10;
        v3[9] = v17;
        const uint8_t v18 = v17 ^ v14;
        const uint8_t v19 = v3[-14];
        v3[6] = v11;
        const uint8_t v20 = v11 ^ v16;
        const uint8_t v21 = v3[-1];
        v3[1] = v13;
        const uint8_t v22 = v13 ^ v19;
        const uint8_t v23 = v3[-5];
        v3[14] = v15;
        const uint8_t v24 = v15 ^ v21;
        const uint8_t v25 = v3[-8];
        v3[10] = v18;
        const uint8_t v26 = v18 ^ v23;
        const uint8_t v27 = v3[-13];
        v3[7] = v20;
        const uint8_t v28 = v20 ^ v25;
        const uint8_t v29 = v3[0];
        v3[2] = v22;
        const uint8_t v30 = v22 ^ v27;
        const uint8_t v31 = v3[-4];
        v3[15] = v24;
        const uint8_t v32 = v24 ^ v29;
        const uint8_t v33 = v3[-12];
        v3[11] = v26;
        v3[3] = v30;
        v3[8] = v28;
        v3[12] = v26 ^ v31;
        v3[4] = v30 ^ v33;
        v3[16] = v32;
        v3 += 16;
    }
}

/* sub_27F0: AES-128 block encrypt; state row-major, in/out column-major */
static void aes_double_block_encrypt(const uint8_t sk[176], const uint8_t in[16], uint8_t out[16])
{
    uint8_t s[16];
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            s[r * 4 + c] = in[c * 4 + r];
    for (int i = 0; i < 16; ++i) s[i] ^= sk[i];
    for (int rnd = 1; rnd <= 10; ++rnd)
    {
        for (int i = 0; i < 16; ++i) s[i] = AES_DOUBLE_S[s[i]];
        {
            uint8_t t[16];
            for (int r = 0; r < 4; ++r)
                for (int c = 0; c < 4; ++c)
                    t[r * 4 + c] = s[r * 4 + ((c + r) & 3)];
            memcpy(s, t, 16);
        }
        if (rnd < 10)
        {
            for (int c = 0; c < 4; ++c)
            {
                const uint8_t a0 = s[c], a1 = s[4 + c], a2 = s[8 + c], a3 = s[12 + c];
                s[c] = aes_double_xtime(a0) ^ (aes_double_xtime(a1) ^ a1) ^ a2 ^ a3;
                s[4 + c] = a0 ^ aes_double_xtime(a1) ^ (aes_double_xtime(a2) ^ a2) ^ a3;
                s[8 + c] = a0 ^ a1 ^ aes_double_xtime(a2) ^ (aes_double_xtime(a3) ^ a3);
                s[12 + c] = (aes_double_xtime(a0) ^ a0) ^ a1 ^ a2 ^ aes_double_xtime(a3);
            }
        }
        for (int i = 0; i < 16; ++i) s[i] ^= sk[16 * rnd + i];
    }
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            out[c * 4 + r] = s[r * 4 + c];
}

/* AES-128 block decrypt (standard inverse rounds, row-major state) */
static void aes_double_block_decrypt(const uint8_t sk[176], const uint8_t in[16], uint8_t out[16])
{
    uint8_t s[16];
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            s[r * 4 + c] = in[c * 4 + r];
    for (int i = 0; i < 16; ++i) s[i] ^= sk[160 + i];
    for (int rnd = 9; rnd >= 1; --rnd)
    {
        /* inv shift rows: t[r][c] = s[r][(c - r) & 3] */
        {
            uint8_t t[16];
            for (int r = 0; r < 4; ++r)
                for (int c = 0; c < 4; ++c)
                    t[r * 4 + c] = s[r * 4 + ((c - r + 4) & 3)];
            memcpy(s, t, 16);
        }
        for (int i = 0; i < 16; ++i) s[i] = AES_DOUBLE_INV_S[s[i]];
        for (int i = 0; i < 16; ++i) s[i] ^= sk[16 * rnd + i];
        /* inv mix columns */
        for (int c = 0; c < 4; ++c)
        {
            const uint8_t a0 = s[c], a1 = s[4 + c], a2 = s[8 + c], a3 = s[12 + c];
            s[c]     = aes_double_mul(0x0e, a0) ^ aes_double_mul(0x0b, a1) ^ aes_double_mul(0x0d, a2) ^ aes_double_mul(0x09, a3);
            s[4 + c] = aes_double_mul(0x09, a0) ^ aes_double_mul(0x0e, a1) ^ aes_double_mul(0x0b, a2) ^ aes_double_mul(0x0d, a3);
            s[8 + c] = aes_double_mul(0x0d, a0) ^ aes_double_mul(0x09, a1) ^ aes_double_mul(0x0e, a2) ^ aes_double_mul(0x0b, a3);
            s[12 + c] = aes_double_mul(0x0b, a0) ^ aes_double_mul(0x0d, a1) ^ aes_double_mul(0x09, a2) ^ aes_double_mul(0x0e, a3);
        }
    }
    {
        uint8_t t[16];
        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c)
                t[r * 4 + c] = s[r * 4 + ((c - r + 4) & 3)];
        memcpy(s, t, 16);
    }
    for (int i = 0; i < 16; ++i) s[i] = AES_DOUBLE_INV_S[s[i]];
    for (int i = 0; i < 16; ++i) s[i] ^= sk[i];
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            out[c * 4 + r] = s[r * 4 + c];
}

/* one layer (sub_27F0) with a4=0: IV-prefixed CBC */
static uint8_t* aes_double_layer_encrypt(const uint8_t key[16], const uint8_t* data, int len,
                                         const uint8_t iv[16], int* out_len)
{
    int total = (len + 15) & ~15;
    if (total == 0) total = 16;
    uint8_t *buf = s_calloc(1, (size_t)total + 32);
    if (!buf) return NULL;
    memcpy(buf + 16, data, (size_t)len);
    memcpy(buf, iv, 16);
    uint8_t sk[176];
    aes_double_key_expand(key, sk);
    uint8_t prev[16];
    memcpy(prev, iv, 16);
    for (int off = 0; off < total; off += 16)
    {
        uint8_t t[16];
        for (int i = 0; i < 16; ++i) t[i] = buf[16 + off + i] ^ prev[i];
        aes_double_block_encrypt(sk, t, buf + 16 + off);
        memcpy(prev, buf + 16 + off, 16);
    }
    *out_len = total + 16;
    return buf;
}

/* one layer decrypt (sub_2ED8 mode 0): IV-prefixed CBC, strip the 16-byte IV prefix */
static uint8_t* aes_double_layer_decrypt(const uint8_t key[16], const uint8_t* data, int len,
                                         int* out_len)
{
    if ((len & 0xF) != 0 || len < 32) return NULL;
    const int total = len - 16;
    uint8_t *buf = s_malloc((size_t)total);
    if (!buf) return NULL;
    uint8_t sk[176];
    aes_double_key_expand(key, sk);
    uint8_t prev[16];
    memcpy(prev, data, 16);
    for (int off = 0; off < total; off += 16)
    {
        uint8_t dec[16];
        aes_double_block_decrypt(sk, data + 16 + off, dec);
        for (int i = 0; i < 16; ++i) buf[off + i] = dec[i] ^ prev[i];
        memcpy(prev, data + 16 + off, 16);
    }
    *out_len = total;
    return buf;
}

/* sub_3444: double-layer encrypt */
static uint8_t* aes_double_encrypt_raw(const uint8_t* key, const uint8_t* iv, const uint8_t* data,
                                       const size_t data_len, size_t* output_len)
{
    int l1 = 0;
    uint8_t *m1 = aes_double_layer_encrypt(key, data, (int)data_len, iv, &l1);
    if (!m1) return NULL;
    int l2 = 0;
    uint8_t *m2 = aes_double_layer_encrypt(key + 16, m1, l1, iv, &l2);
    s_free(m1);
    *output_len = (size_t)l2;
    return m2;
}

/* sub_34D8: double-layer decrypt (layer2 first, then layer1) */
static uint8_t* aes_double_decrypt_raw(const uint8_t* key, const uint8_t* data,
                                       const size_t data_len, size_t* output_len)
{
    int l1 = 0;
    uint8_t *m1 = aes_double_layer_decrypt(key + 16, data, (int)data_len, &l1);
    if (!m1) return NULL;
    int l0 = 0;
    uint8_t *pt = aes_double_layer_decrypt(key, m1, l1, &l0);
    s_free(m1);
    *output_len = (size_t)l0;
    return pt;
}

static char* aes_double_cbc_encrypt(cipher_interface_t* self, const char* text)
{
    if(!self || !text) return NULL;
    const aes_double_cbc_android_data_t* d = self->private_data;
    const size_t text_len = strlen(text);
    size_t out_len=0; uint8_t* out = aes_double_encrypt_raw(d->key, d->iv, (const uint8_t*)text, text_len, &out_len);
    if(!out) return NULL;
    char* hex = bytes_2_hex(out, out_len);
    s_free(out);
    return hex;
}

static char* aes_double_cbc_decrypt(cipher_interface_t* self, const char* hex)
{
    if(!self || !hex) return NULL;
    const aes_double_cbc_android_data_t* d = self->private_data;
    size_t bytes_len=0; uint8_t* bytes = hex_2_bytes(hex, &bytes_len);
    if(!bytes) return NULL;
    size_t out_len=0; uint8_t* out = aes_double_decrypt_raw(d->key, bytes, bytes_len, &out_len);
    s_free(bytes);
    if(!out) return NULL;
    /* zero-padded (not PKCS7): strip trailing zeros, keep as C string */
    while(out_len > 0 && out[out_len - 1] == 0) out_len--;
    char* result = s_malloc(out_len + 1);
    memcpy(result, out, out_len);
    result[out_len] = '\0';
    s_free(out);
    return result;
}

static void aes_double_cbc_destroy(cipher_interface_t* self)
{
    if(self)
    {
        s_free(self->private_data);
        s_free(self);
    }
}

cipher_interface_t* create_aes_double_cbc_cipher(const uint8_t* key, const uint8_t* iv)
{
    if(!key || !iv) return NULL;
    cipher_interface_t* ci = s_calloc(1, sizeof(cipher_interface_t));
    aes_double_cbc_android_data_t* d = s_malloc(sizeof(aes_double_cbc_android_data_t));
    memcpy(d->key, key, AES_DOUBLE_KEY_SIZE);
    memcpy(d->iv, iv, AES_DOUBLE_BLOCK_SIZE);
    ci->encrypt = aes_double_cbc_encrypt;
    ci->decrypt = aes_double_cbc_decrypt;
    ci->destroy = aes_double_cbc_destroy;
    ci->private_data = d;
    return ci;
}
