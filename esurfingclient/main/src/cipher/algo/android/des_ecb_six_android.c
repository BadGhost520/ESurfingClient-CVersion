#include "cipher/CipherInterface.h"
#include "cipher/CipherUtils.h"

#include "utils/simssl/evp.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define DES_SIX_KEY_SIZE 48
#define DES_SIX_BLOCK_SIZE 8

typedef struct {
    uint8_t key[DES_SIX_KEY_SIZE];
} des_ecb_six_android_data_t;

static uint8_t* des_six_encrypt_raw(const uint8_t* key, const uint8_t* data,
                                    const size_t data_len, size_t* output_len)
{
    const int total = (int)((data_len + 7) & ~7);
    uint8_t *out = s_calloc(1, total ? (size_t)total : DES_SIX_BLOCK_SIZE);
    if (!out) return NULL;
    memcpy(out, data, data_len);
    for (int off = 0; off < total; off += 8)
    {
        uint8_t b[8], t[8];
        memcpy(b, out + off, 8);
        simssl_des3_encrypt_block(key + 24, b, t);
        simssl_des3_encrypt_block(key + 0, t, b);
        memcpy(out + off, b, 8);
    }
    *output_len = (size_t)total;
    return out;
}

static uint8_t* des_six_decrypt_raw(const uint8_t* key, const uint8_t* data,
                                    const size_t data_len, size_t* output_len)
{
    if ((data_len & 7) != 0 || data_len < 8) return NULL;
    uint8_t *out = s_malloc(data_len);
    if (!out) return NULL;
    memcpy(out, data, data_len);
    for (size_t off = 0; off < data_len; off += 8)
    {
        uint8_t b[8], t[8];
        memcpy(b, out + off, 8);
        simssl_des3_decrypt_block(key + 0, b, t);
        simssl_des3_decrypt_block(key + 24, t, b);
        memcpy(out + off, b, 8);
    }
    *output_len = data_len;
    return out;
}

static char* des_ecb_six_encrypt(cipher_interface_t* self, const char* text)
{
    if(!self || !text) return NULL;
    const des_ecb_six_android_data_t* d = self->private_data;
    const size_t text_len = strlen(text);
    size_t out_len=0; uint8_t* out = des_six_encrypt_raw(d->key, (const uint8_t*)text, text_len, &out_len);
    if(!out) return NULL;
    char* hex = bytes_2_hex(out, out_len);
    s_free(out);
    return hex;
}

static char* des_ecb_six_decrypt(cipher_interface_t* self, const char* hex)
{
    if(!self || !hex) return NULL;
    const des_ecb_six_android_data_t* d = self->private_data;
    size_t bytes_len=0; uint8_t* bytes = hex_2_bytes(hex, &bytes_len);
    if(!bytes) return NULL;
    size_t out_len=0; uint8_t* out = des_six_decrypt_raw(d->key, bytes, bytes_len, &out_len);
    s_free(bytes);
    if(!out) return NULL;
    while(out_len > 0 && out[out_len - 1] == 0) out_len--;
    char* result = s_malloc(out_len + 1);
    memcpy(result, out, out_len);
    result[out_len] = '\0';
    s_free(out);
    return result;
}

static void des_ecb_six_destroy(cipher_interface_t* self)
{
    if(self)
    {
        s_free(self->private_data);
        s_free(self);
    }
}

cipher_interface_t* create_des_ecb_six_android_cipher(const uint8_t* key)
{
    if(!key) return NULL;
    cipher_interface_t* ci = s_calloc(1, sizeof(cipher_interface_t));
    des_ecb_six_android_data_t* d = s_malloc(sizeof(des_ecb_six_android_data_t));
    memcpy(d->key, key, DES_SIX_KEY_SIZE);
    ci->encrypt = des_ecb_six_encrypt;
    ci->decrypt = des_ecb_six_decrypt;
    ci->destroy = des_ecb_six_destroy;
    ci->private_data = d;
    return ci;
}
