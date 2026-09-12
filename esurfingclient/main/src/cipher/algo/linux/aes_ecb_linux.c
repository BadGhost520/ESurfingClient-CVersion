#include "cipher/CipherInterface.h"
#include "cipher/CipherUtils.h"

#include "utils/simssl/evp.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef struct {
    uint8_t key1[16];
    uint8_t key2[16];
} aes_ecb_linux_ctx_t;

static void aes128_encrypt_block(const uint8_t* in, uint8_t* out, const uint8_t* key16)
{
    simssl_aes128_encrypt_block(key16, in, out);
}

static void aes128_decrypt_block(const uint8_t* in, uint8_t* out, const uint8_t* key16)
{
    simssl_aes128_decrypt_block(key16, in, out);
}
static char* aes_ecb_encrypt(cipher_interface_t* self, const char* text)
{
    if (!self || !text) return NULL;
    const aes_ecb_linux_ctx_t* ctx = self->private_data;
    const size_t len = s_strlen(text);
    const uint8_t* input = (const uint8_t*)text;
    size_t padded_len = 0;
    uint8_t* padded = pad_2_multiple(input, len, 16, &padded_len);
    if (!padded) return NULL;
    uint8_t* out = s_malloc(padded_len);
    for (size_t i = 0; i < padded_len; i += 16)
    {
        uint8_t tmp[16];
        aes128_encrypt_block(padded + i, tmp, ctx->key2);
        aes128_encrypt_block(tmp, out + i, ctx->key1);
    }
    char* hex = bytes_2_hex(out, padded_len);
    s_free(padded);
    s_free(out);
    return hex;
}

static char* aes_ecb_decrypt(cipher_interface_t* self, const char* hex)
{
    if (!self || !hex) return NULL;
    const aes_ecb_linux_ctx_t* ctx = self->private_data;
    size_t in_len = 0;
    uint8_t* in = hex_2_bytes(hex, &in_len);
    if (!in || in_len % 16 != 0)
    {
        s_free(in); return NULL;
    }
    uint8_t* out = s_malloc(in_len);
    for (size_t i = 0; i < in_len; i += 16)
    {
        uint8_t tmp[16];
        aes128_decrypt_block(in + i, tmp, ctx->key1);
        aes128_decrypt_block(tmp, out + i, ctx->key2);
    }
    size_t plain_len = in_len;
    while (plain_len > 0 && out[plain_len - 1] == 0x00) plain_len--;
    char* text = s_malloc(plain_len + 1);
    memcpy(text, out, plain_len);
    text[plain_len] = '\0';
    s_free(in);
    s_free(out);
    return text;
}

static void aes_ecb_destroy(cipher_interface_t* self)
{
    if (!self) return;
    if (self->private_data) s_free(self->private_data);
    s_free(self);
}

cipher_interface_t* create_aes_ecb_linux_cipher(const uint8_t* key1, const uint8_t* key2)
{
    if (!key1 || !key2) return NULL;
    cipher_interface_t* ci = s_calloc(1, sizeof(cipher_interface_t));
    aes_ecb_linux_ctx_t* ctx = s_calloc(1, sizeof(aes_ecb_linux_ctx_t));
    memcpy(ctx->key1, key1, 16);
    memcpy(ctx->key2, key2, 16);
    ci->encrypt = aes_ecb_encrypt;
    ci->decrypt = aes_ecb_decrypt;
    ci->destroy = aes_ecb_destroy;
    ci->private_data = ctx;
    return ci;
}