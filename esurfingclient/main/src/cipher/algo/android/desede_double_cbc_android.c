#include "cipher/CipherInterface.h"
#include "cipher/CipherUtils.h"

#include "utils/simssl/evp.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define DESEDE_DOUBLE_KEY_SIZE 48
#define DESEDE_DOUBLE_BLOCK_SIZE 8

typedef struct {
    uint8_t key[DESEDE_DOUBLE_KEY_SIZE];
    uint8_t iv[DESEDE_DOUBLE_BLOCK_SIZE];
} desede_double_cbc_android_data_t;

static uint8_t* desede_double_encrypt_raw(const uint8_t* key, const uint8_t* iv, const uint8_t* data,
                                          const size_t data_len, size_t* output_len)
{
    const int total = (int)((data_len + 7) & ~7);
    uint8_t *out = s_calloc(1, total ? (size_t)total : DESEDE_DOUBLE_BLOCK_SIZE);
    if (!out) return NULL;
    memcpy(out, data, data_len);
    uint8_t prev[8];
    memcpy(prev, iv, 8);
    for (int off = 0; off < total; off += 8)
    {
        uint8_t t[8];
        for (int i = 0; i < 8; ++i) t[i] = out[off + i] ^ prev[i];
        simssl_des3_encrypt_block(key + 24, t, out + off);
        memcpy(prev, out + off, 8);
    }
    memcpy(prev, iv, 8);
    for (int off = 0; off < total; off += 8)
    {
        uint8_t t[8];
        for (int i = 0; i < 8; ++i) t[i] = out[off + i] ^ prev[i];
        simssl_des3_encrypt_block(key + 0, t, out + off);
        memcpy(prev, out + off, 8);
    }
    *output_len = (size_t)total;
    return out;
}

static uint8_t* desede_double_decrypt_raw(const uint8_t* key, const uint8_t* iv, const uint8_t* data,
                                          const size_t data_len, size_t* output_len)
{
    if ((data_len & 7) != 0 || data_len < 8) return NULL;
    uint8_t *buf = s_malloc(data_len);
    if (!buf) return NULL;
    memcpy(buf, data, data_len);
    for (int off = (int)data_len - 8; off >= 0; off -= 8)
    {
        uint8_t t3[8];
        simssl_des3_decrypt_block(key + 0, buf + off, t3);
        const uint8_t *prev = (off == 0) ? iv : buf + off - 8;
        for (int i = 0; i < 8; ++i) buf[off + i] = t3[i] ^ prev[i];
    }
    for (int off = (int)data_len - 8; off >= 0; off -= 8)
    {
        uint8_t t3[8];
        simssl_des3_decrypt_block(key + 24, buf + off, t3);
        const uint8_t *prev = (off == 0) ? iv : buf + off - 8;
        for (int i = 0; i < 8; ++i) buf[off + i] = t3[i] ^ prev[i];
    }
    *output_len = data_len;
    return buf;
}

static char* desede_double_cbc_encrypt(cipher_interface_t* self, const char* text)
{
    if(!self || !text) return NULL;
    const desede_double_cbc_android_data_t* d = self->private_data;
    const size_t text_len = strlen(text);
    size_t out_len=0; uint8_t* out = desede_double_encrypt_raw(d->key, d->iv, (const uint8_t*)text, text_len, &out_len);
    if(!out) return NULL;
    char* hex = bytes_2_hex(out, out_len);
    s_free(out);
    return hex;
}

static char* desede_double_cbc_decrypt(cipher_interface_t* self, const char* hex)
{
    if(!self || !hex) return NULL;
    const desede_double_cbc_android_data_t* d = self->private_data;
    size_t bytes_len=0; uint8_t* bytes = hex_2_bytes(hex, &bytes_len);
    if(!bytes) return NULL;
    size_t out_len=0; uint8_t* out = desede_double_decrypt_raw(d->key, d->iv, bytes, bytes_len, &out_len);
    s_free(bytes);
    if(!out) return NULL;
    while(out_len > 0 && out[out_len - 1] == 0) out_len--;
    char* result = s_malloc(out_len + 1);
    memcpy(result, out, out_len);
    result[out_len] = '\0';
    s_free(out);
    return result;
}

static void desede_double_cbc_destroy(cipher_interface_t* self)
{
    if(self)
    {
        s_free(self->private_data);
        s_free(self);
    }
}

cipher_interface_t* create_desede_double_cbc_android_cipher(const uint8_t* key, const uint8_t* iv)
{
    if(!key || !iv) return NULL;
    cipher_interface_t* ci = s_calloc(1, sizeof(cipher_interface_t));
    desede_double_cbc_android_data_t* d = s_malloc(sizeof(desede_double_cbc_android_data_t));
    memcpy(d->key, key, DESEDE_DOUBLE_KEY_SIZE);
    memcpy(d->iv, iv, DESEDE_DOUBLE_BLOCK_SIZE);
    ci->encrypt = desede_double_cbc_encrypt;
    ci->decrypt = desede_double_cbc_decrypt;
    ci->destroy = desede_double_cbc_destroy;
    ci->private_data = d;
    return ci;
}
