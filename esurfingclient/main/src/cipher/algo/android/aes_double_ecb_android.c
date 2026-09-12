#include "cipher/CipherInterface.h"
#include "cipher/CipherUtils.h"

#include "utils/simssl/evp.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define AES_DOUBLE_ECB_KEY_SIZE 32
#define AES_DOUBLE_ECB_BLOCK_SIZE 16

typedef struct {
    uint8_t key[AES_DOUBLE_ECB_KEY_SIZE];
} aes_double_ecb_android_data_t;

static uint8_t* aes_ecb_layer_encrypt(const uint8_t key[16], const uint8_t* data, int len, int* out_len)
{
    int total = (len + 15) & ~15;
    if (total == 0) total = 16;
    uint8_t *buf = s_calloc(1, (size_t)total);
    if (!buf) return NULL;
    memcpy(buf, data, (size_t)len);
    for (int off = 0; off < total; off += 16)
        simssl_aes128_encrypt_block(key, buf + off, buf + off);
    *out_len = total;
    return buf;
}

static uint8_t* aes_ecb_layer_decrypt(const uint8_t key[16], const uint8_t* data, int len, int* out_len)
{
    if ((len & 0xF) != 0 || len < 16) return NULL;
    uint8_t *buf = s_malloc((size_t)len);
    if (!buf) return NULL;
    memcpy(buf, data, (size_t)len);
    for (int off = 0; off < len; off += 16)
        simssl_aes128_decrypt_block(key, buf + off, buf + off);
    *out_len = len;
    return buf;
}

static uint8_t* aes_ecb_double_encrypt_raw(const uint8_t* key, const uint8_t* data,
                                           const size_t data_len, size_t* output_len)
{
    int l1 = 0;
    uint8_t *m1 = aes_ecb_layer_encrypt(key, data, (int)data_len, &l1);
    if (!m1) return NULL;
    int l2 = 0;
    uint8_t *m2 = aes_ecb_layer_encrypt(key + 16, m1, l1, &l2);
    s_free(m1);
    *output_len = (size_t)l2;
    return m2;
}

static uint8_t* aes_ecb_double_decrypt_raw(const uint8_t* key, const uint8_t* data,
                                           const size_t data_len, size_t* output_len)
{
    int l1 = 0;
    uint8_t *m1 = aes_ecb_layer_decrypt(key + 16, data, (int)data_len, &l1);
    if (!m1) return NULL;
    int l0 = 0;
    uint8_t *pt = aes_ecb_layer_decrypt(key, m1, l1, &l0);
    s_free(m1);
    *output_len = (size_t)l0;
    return pt;
}

static char* aes_double_ecb_encrypt(cipher_interface_t* self, const char* text)
{
    if(!self || !text) return NULL;
    const aes_double_ecb_android_data_t* d = self->private_data;
    const size_t text_len = strlen(text);
    size_t out_len=0; uint8_t* out = aes_ecb_double_encrypt_raw(d->key, (const uint8_t*)text, text_len, &out_len);
    if(!out) return NULL;
    char* hex = bytes_2_hex(out, out_len);
    s_free(out);
    return hex;
}

static char* aes_double_ecb_decrypt(cipher_interface_t* self, const char* hex)
{
    if(!self || !hex) return NULL;
    const aes_double_ecb_android_data_t* d = self->private_data;
    size_t bytes_len=0; uint8_t* bytes = hex_2_bytes(hex, &bytes_len);
    if(!bytes) return NULL;
    size_t out_len=0; uint8_t* out = aes_ecb_double_decrypt_raw(d->key, bytes, bytes_len, &out_len);
    s_free(bytes);
    if(!out) return NULL;
    while(out_len > 0 && out[out_len - 1] == 0) out_len--;
    char* result = s_malloc(out_len + 1);
    memcpy(result, out, out_len);
    result[out_len] = '\0';
    s_free(out);
    return result;
}

static void aes_double_ecb_destroy(cipher_interface_t* self)
{
    if(self)
    {
        s_free(self->private_data);
        s_free(self);
    }
}

cipher_interface_t* create_aes_double_ecb_android_cipher(const uint8_t* key)
{
    if(!key) return NULL;
    cipher_interface_t* ci = s_calloc(1, sizeof(cipher_interface_t));
    aes_double_ecb_android_data_t* d = s_malloc(sizeof(aes_double_ecb_android_data_t));
    memcpy(d->key, key, AES_DOUBLE_ECB_KEY_SIZE);
    ci->encrypt = aes_double_ecb_encrypt;
    ci->decrypt = aes_double_ecb_decrypt;
    ci->destroy = aes_double_ecb_destroy;
    ci->private_data = d;
    return ci;
}
