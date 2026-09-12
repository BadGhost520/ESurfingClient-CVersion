#include "cipher/CipherInterface.h"
#include "cipher/CipherUtils.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define TEA_TRIPLE_CBC_KEY_SIZE 48
#define TEA_TRIPLE_CBC_BLOCK_SIZE 8
#define TEA_CBC_DELTA 0x61C88647u

typedef struct {
    uint8_t key[TEA_TRIPLE_CBC_KEY_SIZE];
    uint8_t iv[TEA_TRIPLE_CBC_BLOCK_SIZE];
} tea_triple_cbc_android_data_t;

static uint32_t tea_cbc_bswap32(uint32_t x)
{
    return ((x & 0xFF) << 24) | ((x & 0xFF00) << 8) | ((x >> 8) & 0xFF00) | (x >> 24);
}

static void tea_cbc_enc_layer(uint32_t *v0, uint32_t *v1, const uint32_t k[4])
{
    uint32_t a = *v0, b = *v1;
    uint32_t sum = 0;
    do
    {
        a += (b ^ sum) + k[sum & 3] + ((b << 4) ^ (b >> 5));
        sum -= TEA_CBC_DELTA;
        b += k[(sum >> 11) & 3] + (a ^ sum) + ((a << 4) ^ (a >> 5));
    } while (sum != 0xC6EF3720u);
    *v0 = a;
    *v1 = b;
}

static void tea_cbc_dec_layer(uint32_t *v0, uint32_t *v1, const uint32_t k[4])
{
    uint32_t a = *v0, b = *v1;
    uint32_t sum = 0xC6EF3720u;
    uint32_t kidx = 0x28B7BD67u;
    do
    {
        b -= (a ^ sum) + ((a << 4) ^ (a >> 5)) + k[(sum >> 11) & 3];
        sum += TEA_CBC_DELTA;
        a -= k[kidx & 3] + (b ^ kidx) + ((b << 4) ^ (b >> 5));
        kidx += TEA_CBC_DELTA;
    } while (sum != 0);
    *v0 = a;
    *v1 = b;
}

static void tea_cbc_load_key_layers(const uint8_t key[48], uint32_t k[3][4])
{
    for (int layer = 0; layer < 3; ++layer)
        for (int i = 0; i < 4; ++i)
            k[layer][i] = tea_cbc_bswap32(((const uint32_t *)key)[layer * 4 + i]);
}

static uint8_t* tea_cbc_encrypt_raw(const uint8_t* key, const uint8_t* iv, const uint8_t* data,
                                    const size_t data_len, size_t* output_len)
{
    int total = (int)((data_len + 7) & ~7);
    if (total == 0) total = TEA_TRIPLE_CBC_BLOCK_SIZE;
    uint8_t *buf = s_calloc(1, (size_t)total);
    if (!buf) return NULL;
    memcpy(buf, data, data_len);
    uint32_t k[3][4];
    tea_cbc_load_key_layers(key, k);
    uint8_t prev[TEA_TRIPLE_CBC_BLOCK_SIZE];
    memcpy(prev, iv, TEA_TRIPLE_CBC_BLOCK_SIZE);
    for (int off = 0; off < total; off += 8)
    {
        uint32_t v0 = tea_cbc_bswap32(((const uint32_t *)buf)[off / 4]) ^ tea_cbc_bswap32(((const uint32_t *)prev)[0]);
        uint32_t v1 = tea_cbc_bswap32(((const uint32_t *)buf)[off / 4 + 1]) ^ tea_cbc_bswap32(((const uint32_t *)prev)[1]);
        tea_cbc_enc_layer(&v0, &v1, k[2]);
        tea_cbc_enc_layer(&v0, &v1, k[1]);
        tea_cbc_enc_layer(&v0, &v1, k[0]);
        ((uint32_t *)buf)[off / 4] = tea_cbc_bswap32(v0);
        ((uint32_t *)buf)[off / 4 + 1] = tea_cbc_bswap32(v1);
        memcpy(prev, buf + off, TEA_TRIPLE_CBC_BLOCK_SIZE);
    }
    *output_len = (size_t)total;
    return buf;
}

static uint8_t* tea_cbc_decrypt_raw(const uint8_t* key, const uint8_t* iv, const uint8_t* data,
                                    const size_t data_len, size_t* output_len)
{
    if ((data_len & 7) != 0 || data_len < 8) return NULL;
    uint8_t *buf = s_malloc(data_len);
    if (!buf) return NULL;
    memcpy(buf, data, data_len);
    uint32_t k[3][4];
    tea_cbc_load_key_layers(key, k);
    uint8_t prev[TEA_TRIPLE_CBC_BLOCK_SIZE];
    memcpy(prev, iv, TEA_TRIPLE_CBC_BLOCK_SIZE);
    for (size_t off = 0; off < data_len; off += 8)
    {
        uint32_t v0 = tea_cbc_bswap32(((const uint32_t *)buf)[off / 4]);
        uint32_t v1 = tea_cbc_bswap32(((const uint32_t *)buf)[off / 4 + 1]);
        tea_cbc_dec_layer(&v0, &v1, k[0]);
        tea_cbc_dec_layer(&v0, &v1, k[1]);
        tea_cbc_dec_layer(&v0, &v1, k[2]);
        ((uint32_t *)buf)[off / 4] = tea_cbc_bswap32(v0 ^ tea_cbc_bswap32(((const uint32_t *)prev)[0]));
        ((uint32_t *)buf)[off / 4 + 1] = tea_cbc_bswap32(v1 ^ tea_cbc_bswap32(((const uint32_t *)prev)[1]));
        memcpy(prev, data + off, TEA_TRIPLE_CBC_BLOCK_SIZE);
    }
    *output_len = data_len;
    return buf;
}

static char* tea_triple_cbc_encrypt(cipher_interface_t* self, const char* text)
{
    if(!self || !text) return NULL;
    const tea_triple_cbc_android_data_t* d = self->private_data;
    const size_t text_len = strlen(text);
    size_t out_len=0; uint8_t* out = tea_cbc_encrypt_raw(d->key, d->iv, (const uint8_t*)text, text_len, &out_len);
    if(!out) return NULL;
    char* hex = bytes_2_hex(out, out_len);
    s_free(out);
    return hex;
}

static char* tea_triple_cbc_decrypt(cipher_interface_t* self, const char* hex)
{
    if(!self || !hex) return NULL;
    const tea_triple_cbc_android_data_t* d = self->private_data;
    size_t bytes_len=0; uint8_t* bytes = hex_2_bytes(hex, &bytes_len);
    if(!bytes) return NULL;
    size_t out_len=0; uint8_t* out = tea_cbc_decrypt_raw(d->key, d->iv, bytes, bytes_len, &out_len);
    s_free(bytes);
    if(!out) return NULL;
    while(out_len > 0 && out[out_len - 1] == 0) out_len--;
    char* result = s_malloc(out_len + 1);
    memcpy(result, out, out_len);
    result[out_len] = '\0';
    s_free(out);
    return result;
}

static void tea_triple_cbc_destroy(cipher_interface_t* self)
{
    if(self)
    {
        s_free(self->private_data);
        s_free(self);
    }
}

cipher_interface_t* create_tea_triple_cbc_android_cipher(const uint8_t* key, const uint8_t* iv)
{
    if(!key || !iv) return NULL;
    cipher_interface_t* ci = s_calloc(1, sizeof(cipher_interface_t));
    tea_triple_cbc_android_data_t* d = s_malloc(sizeof(tea_triple_cbc_android_data_t));
    memcpy(d->key, key, TEA_TRIPLE_CBC_KEY_SIZE);
    memcpy(d->iv, iv, TEA_TRIPLE_CBC_BLOCK_SIZE);
    ci->encrypt = tea_triple_cbc_encrypt;
    ci->decrypt = tea_triple_cbc_decrypt;
    ci->destroy = tea_triple_cbc_destroy;
    ci->private_data = d;
    return ci;
}
