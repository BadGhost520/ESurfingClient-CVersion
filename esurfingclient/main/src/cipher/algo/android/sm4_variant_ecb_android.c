#include "cipher/CipherInterface.h"
#include "cipher/CipherUtils.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define SM4_VARIANT_ECB_BLOCK_SIZE 16
#define SM4_VARIANT_ECB_KEY_SIZE 16

typedef struct {
    uint8_t key[SM4_VARIANT_ECB_KEY_SIZE];
} sm4_variant_ecb_android_data_t;

static const uint32_t SM4_VARIANT_ECB_FK[4] = {0xA3B1BAC6, 0x56AA3350, 0x677D9197, 0xB27022DC};
static const uint32_t SM4_VARIANT_ECB_CK[32] = {
    0x00070E15,0x1C232A31,0x383F464D,0x545B6269,0x70777E85,0x8C939AA1,0xA8AFB6BD,0xC4CBD2D9,
    0xE0E7EEF5,0xFC030A11,0x181F262D,0x343B4249,0x50575E65,0x6C737A81,0x888F969D,0xA4ABB2B9,
    0xC0C7CED5,0xDCE3EAF1,0xF8FF060D,0x141B2229,0x30373E45,0x4C535A61,0x686F767D,0x848B9299,
    0xA0A7AEB5,0xBCC3CAD1,0xD8DFE6ED,0xF4FB0209,0x10171E25,0x2C333A41,0x484F565D,0x646B7279
};
static const uint8_t SM4_VARIANT_ECB_S[256] = {
    0xd6,0x90,0xe9,0xfe,0xcc,0xe1,0x3d,0xb7,0x16,0xb6,0x14,0xc2,0x28,0xfb,0x2c,0x05,0x2b,0x67,0x9a,0x76,0x2a,0xbe,0x04,0xc3,0xaa,0x44,0x13,0x26,0x49,0x86,0x06,0x99,0x9c,0x42,0x50,0xf4,0x91,0xef,0x98,0x7a,0x33,0x54,0x0b,0x43,0xed,0xcf,0xac,0x62,0xe4,0xb3,0x1c,0xa9,0xc9,0x08,0xe8,0x95,0x80,0xdf,0x94,0xfa,0x75,0x8f,0x3f,0xa6,0x47,0x07,0xa7,0xfc,0xf3,0x73,0x17,0xba,0x83,0x59,0x3c,0x19,0xe6,0x85,0x4f,0xa8,0x68,0x6b,0x81,0xb2,0x71,0x64,0xda,0x8b,0xf8,0xeb,0x0f,0x4b,0x70,0x56,0x9d,0x35,0x1e,0x24,0x0e,0x5e,0x63,0x58,0xd1,0xa2,0x25,0x22,0x7c,0x3b,0x01,0x21,0x78,0x87,0xd4,0x00,0x46,0x57,0x9f,0xd3,0x27,0x52,0x4c,0x36,0x02,0xe7,0xa0,0xc4,0xc8,0x9e,0xea,0xbf,0x8a,0xd2,0x40,0xc7,0x38,0xb5,0xa3,0xf7,0xf2,0xce,0xf9,0x61,0x15,0xa1,0xe0,0xae,0x5d,0xa4,0x9b,0x34,0x1a,0x55,0xad,0x93,0x32,0x30,0xf5,0x8c,0xb1,0xe3,0x1d,0xf6,0xe2,0x2e,0x82,0x66,0xca,0x60,0xc0,0x29,0x23,0xab,0x0d,0x53,0x4e,0x6f,0xd5,0xdb,0x37,0x45,0xde,0xfd,0x8e,0x2f,0x03,0xff,0x6a,0x72,0x6d,0x6c,0x5b,0x51,0x8d,0x1b,0xaf,0x92,0xbb,0xdd,0xbc,0x7f,0x11,0xd9,0x5c,0x41,0x1f,0x10,0x5a,0xd8,0x0a,0xc1,0x31,0x88,0xa5,0xcd,0x7b,0xbd,0x2d,0x74,0xd0,0x12,0xb8,0xe5,0xb4,0xb0,0x89,0x69,0x97,0x4a,0x0c,0x96,0x77,0x7e,0x65,0xb9,0xf1,0x09,0xc5,0x6e,0xc6,0x84,0x18,0xf0,0x7d,0xec,0x3a,0xdc,0x4d,0x20,0x79,0xee,0x5f,0x3e,0xd7,0xcb,0x39,0x48
};

static uint32_t sm4_variant_ecb_bswap32(uint32_t x)
{
    return ((x & 0xFF) << 24) | ((x & 0xFF00) << 8) | ((x >> 8) & 0xFF00) | (x >> 24);
}

static uint32_t sm4_variant_ecb_rd32be(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}

static void sm4_variant_ecb_wr32be(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);  p[3] = (uint8_t)v;
}

static void sm4_variant_ecb_keysched(const uint8_t key[16], uint32_t rk[32], int mode)
{
    uint32_t K[4];
    for (int i = 0; i < 4; ++i)
        K[i] = sm4_variant_ecb_bswap32(((const uint32_t *)key)[i]) ^ SM4_VARIANT_ECB_FK[i];
    for (int i = 0; i < 32; ++i)
    {
        const uint32_t t = K[1] ^ K[2] ^ K[3] ^ SM4_VARIANT_ECB_CK[i];
        const uint32_t s0 = SM4_VARIANT_ECB_S[t >> 24], s1 = SM4_VARIANT_ECB_S[(t >> 16) & 0xFF],
                 s2 = SM4_VARIANT_ECB_S[(t >> 8) & 0xFF], s3 = SM4_VARIANT_ECB_S[t & 0xFF];
        const uint32_t w4 = (s0 << 24) | (s1 << 16);
        const uint32_t w1 = w4 | (s2 << 8);
        const uint32_t B = w1 | s3;
        const uint32_t e1 = ((B << 13) | (w4 >> 19)) & 0xFFFFFFFFu;
        const uint32_t e2 = ((B << 23) | (w1 >> 9)) & 0xFFFFFFFFu;
        rk[i] = B ^ K[0] ^ e1 ^ e2;
        K[0] = K[1]; K[1] = K[2]; K[2] = K[3]; K[3] = rk[i];
    }
    if (mode == 0)
    {
        for (int i = 0; i < 16; ++i)
        {
            const uint32_t t = rk[i];
            rk[i] = rk[31 - i];
            rk[31 - i] = t;
        }
    }
}

static void sm4_variant_ecb_block(const uint32_t rk[32], const uint8_t in[16], uint8_t out[16])
{
    uint32_t X[4] = { sm4_variant_ecb_rd32be(in), sm4_variant_ecb_rd32be(in + 4),
                      sm4_variant_ecb_rd32be(in + 8), sm4_variant_ecb_rd32be(in + 12) };
    uint32_t store[32];
    for (int i = 0; i < 32; ++i)
    {
        const uint32_t t = X[1] ^ X[2] ^ X[3] ^ rk[i];
        const uint32_t s0 = SM4_VARIANT_ECB_S[t >> 24], s1 = SM4_VARIANT_ECB_S[(t >> 16) & 0xFF],
                 s2 = SM4_VARIANT_ECB_S[(t >> 8) & 0xFF], s3 = SM4_VARIANT_ECB_S[t & 0xFF];
        const uint32_t w4 = (s0 << 24) | (s1 << 16);
        const uint32_t w1 = w4 | (s2 << 8);
        const uint32_t B = w1 | s3;
        const uint32_t e0 = (s0 >> 6) | ((B & 0x3FFFFFFF) << 2);
        const uint32_t e1 = ((B << 24) | (w1 >> 8)) & 0xFFFFFFFFu;
        const uint32_t e2 = ((B << 10) | (w4 >> 22)) & 0xFFFFFFFFu;
        const uint32_t e3 = ((B << 18) | (w1 >> 14)) & 0xFFFFFFFFu;
        const uint32_t nx = B ^ X[0] ^ e1 ^ e0 ^ e2 ^ e3;
        store[i] = nx;
        X[0] = X[1]; X[1] = X[2]; X[2] = X[3]; X[3] = nx;
    }
    sm4_variant_ecb_wr32be(out, store[31]);
    sm4_variant_ecb_wr32be(out + 4, store[30]);
    sm4_variant_ecb_wr32be(out + 8, store[29]);
    sm4_variant_ecb_wr32be(out + 12, store[28]);
}

static uint8_t* sm4_variant_ecb_encrypt_raw(const uint8_t* key, const uint8_t* data,
                                            const size_t data_len, size_t* output_len)
{
    const int pad = (int)(SM4_VARIANT_ECB_BLOCK_SIZE - (data_len & 0xF));
    const int total = (int)data_len + pad;
    uint8_t *buf = s_calloc(1, (size_t)total);
    if (!buf) return NULL;
    memcpy(buf, data, data_len);
    memset(buf + data_len, pad, (size_t)pad);
    uint32_t rk[32];
    sm4_variant_ecb_keysched(key, rk, 1);
    for (int off = 0; off < total; off += 16)
        sm4_variant_ecb_block(rk, buf + off, buf + off);
    *output_len = (size_t)total;
    return buf;
}

static uint8_t* sm4_variant_ecb_decrypt_raw(const uint8_t* key, const uint8_t* data,
                                            const size_t data_len, size_t* output_len)
{
    if ((data_len & 0xF) != 0 || data_len < 16) return NULL;
    uint8_t *buf = s_malloc(data_len);
    if (!buf) return NULL;
    memcpy(buf, data, data_len);
    uint32_t rk[32];
    sm4_variant_ecb_keysched(key, rk, 0);
    for (size_t off = 0; off < data_len; off += 16)
        sm4_variant_ecb_block(rk, buf + off, buf + off);
    const int pad = buf[data_len - 1];
    if (pad < 1 || pad > 16) { s_free(buf); return NULL; }
    *output_len = data_len - (size_t)pad;
    return buf;
}

static char* sm4_variant_ecb_encrypt(cipher_interface_t* self, const char* text)
{
    if(!self || !text) return NULL;
    const sm4_variant_ecb_android_data_t* d = self->private_data;
    const size_t text_len = strlen(text);
    size_t out_len=0; uint8_t* out = sm4_variant_ecb_encrypt_raw(d->key, (const uint8_t*)text, text_len, &out_len);
    if(!out) return NULL;
    char* hex = bytes_2_hex(out, out_len);
    s_free(out);
    return hex;
}

static char* sm4_variant_ecb_decrypt(cipher_interface_t* self, const char* hex)
{
    if(!self || !hex) return NULL;
    const sm4_variant_ecb_android_data_t* d = self->private_data;
    size_t bytes_len=0; uint8_t* bytes = hex_2_bytes(hex, &bytes_len);
    if(!bytes) return NULL;
    size_t out_len=0; uint8_t* out = sm4_variant_ecb_decrypt_raw(d->key, bytes, bytes_len, &out_len);
    s_free(bytes);
    if(!out) return NULL;
    char* result = s_malloc(out_len + 1);
    memcpy(result, out, out_len);
    result[out_len] = '\0';
    s_free(out);
    return result;
}

static void sm4_variant_ecb_destroy(cipher_interface_t* self)
{
    if(self)
    {
        s_free(self->private_data);
        s_free(self);
    }
}

cipher_interface_t* create_sm4_variant_ecb_android_cipher(const uint8_t* key)
{
    if(!key) return NULL;
    cipher_interface_t* ci = s_calloc(1, sizeof(cipher_interface_t));
    sm4_variant_ecb_android_data_t* d = s_malloc(sizeof(sm4_variant_ecb_android_data_t));
    memcpy(d->key, key, SM4_VARIANT_ECB_KEY_SIZE);
    ci->encrypt = sm4_variant_ecb_encrypt;
    ci->decrypt = sm4_variant_ecb_decrypt;
    ci->destroy = sm4_variant_ecb_destroy;
    ci->private_data = d;
    return ci;
}
