#include "cipher/CipherInterface.h"
#include "cipher/CipherUtils.h"

#include "utils/simssl/evp.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint8_t key[48];
} des_ecb_six_pc_ctx_t;

static char* des_ecb_six_encrypt(cipher_interface_t* self, const char* text)
{
    if (!self || !text) return NULL;
    const des_ecb_six_pc_ctx_t* ctx = self->private_data;
    const size_t len = s_strlen(text);
    size_t padded_len = 0;
    uint8_t* padded = pad_2_multiple((const uint8_t*)text, len, 8, &padded_len);
    if (!padded) return NULL;
    uint8_t* out = s_malloc(padded_len);
    for (size_t i = 0; i < padded_len; i += 8)
    {
        uint8_t b[8], t[8];
        memcpy(b, padded + i, 8);
        simssl_des3_encrypt_block(ctx->key + 24, b, t);
        simssl_des3_encrypt_block(ctx->key + 0, t, b);
        memcpy(out + i, b, 8);
    }
    char* hex = bytes_2_hex(out, padded_len);
    s_free(padded);
    s_free(out);
    return hex;
}

static char* des_ecb_six_decrypt(cipher_interface_t* self, const char* hex)
{
    if (!self || !hex) return NULL;
    const des_ecb_six_pc_ctx_t* ctx = self->private_data;
    size_t in_len = 0;
    uint8_t* in = hex_2_bytes(hex, &in_len);
    if (!in || (in_len % 8) != 0)
    {
        s_free(in); return NULL;
    }
    uint8_t* out = s_malloc(in_len);
    for (size_t i = 0; i < in_len; i += 8)
    {
        uint8_t b[8], t[8];
        memcpy(b, in + i, 8);
        simssl_des3_decrypt_block(ctx->key + 0, b, t);
        simssl_des3_decrypt_block(ctx->key + 24, t, b);
        memcpy(out + i, b, 8);
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

static void des_ecb_six_destroy(cipher_interface_t* self)
{
    if (!self) return;
    if (self->private_data) s_free(self->private_data);
    s_free(self);
}

cipher_interface_t* create_des_ecb_six_linux_cipher(
    const uint8_t* key0, const uint8_t* key1,
    const uint8_t* key2, const uint8_t* key3,
    const uint8_t* key4, const uint8_t* key5
)
{
    if (!key0 || !key1 || !key2 || !key3 || !key4 || !key5) return NULL;
    cipher_interface_t* ci = s_calloc(1, sizeof(cipher_interface_t));
    des_ecb_six_pc_ctx_t* ctx = s_calloc(1, sizeof(des_ecb_six_pc_ctx_t));
    memcpy(ctx->key + 0,  key0, 8);
    memcpy(ctx->key + 8,  key1, 8);
    memcpy(ctx->key + 16, key2, 8);
    memcpy(ctx->key + 24, key3, 8);
    memcpy(ctx->key + 32, key4, 8);
    memcpy(ctx->key + 40, key5, 8);
    ci->encrypt = des_ecb_six_encrypt;
    ci->decrypt = des_ecb_six_decrypt;
    ci->destroy = des_ecb_six_destroy;
    ci->private_data = ctx;
    return ci;
}