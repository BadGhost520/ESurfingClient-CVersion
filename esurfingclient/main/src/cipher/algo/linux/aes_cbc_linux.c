#include "cipher/CipherInterface.h"
#include "cipher/CipherUtils.h"

#include "utils/simssl/evp.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef struct {
    uint8_t key1[16];
    uint8_t key2[16];
} aes_cbc_linux_ctx_t;

static void aes128_encrypt_block(const uint8_t* in, uint8_t* out, const uint8_t* key16)
{
    simssl_aes128_encrypt_block(key16, in, out);
}

static void aes128_decrypt_block(const uint8_t* in, uint8_t* out, const uint8_t* key16)
{
    simssl_aes128_decrypt_block(key16, in, out);
}
static void cbc_encrypt(const uint8_t* in, uint8_t* out, size_t len, const uint8_t* key16, const uint8_t iv[16])   /* [REFACTOR] rk -> key16(16 字节原始密钥) */
{
    uint8_t prev[16]; memcpy(prev, iv, 16);
    for (size_t i = 0; i < len; i += 16)
    {
        uint8_t x[16]; for (int j=0;j<16;j++) x[j] = in[i+j] ^ prev[j];
        aes128_encrypt_block(x, out + i, key16);
        memcpy(prev, out + i, 16);
    }
}

static void cbc_decrypt(const uint8_t* in, uint8_t* out, size_t len, const uint8_t* key16, const uint8_t iv[16])   /* [REFACTOR] rk -> key16(16 字节原始密钥) */
{
    uint8_t prev[16]; memcpy(prev, iv, 16);
    for (size_t i = 0; i < len; i += 16)
    {
        uint8_t x[16]; aes128_decrypt_block(in + i, x, key16);
        for (int j=0;j<16;j++) out[i+j] = x[j] ^ prev[j];
        memcpy(prev, in + i, 16);
    }
}

static char* aes_cbc_encrypt(cipher_interface_t* self, const char* text)
{
    if (!self || !text) return NULL;
    const aes_cbc_linux_ctx_t* ctx = self->private_data;
    const size_t len = s_strlen(text);
    const uint8_t* input = (const uint8_t*)text;
    size_t padded_len = 0;
    uint8_t* padded = pad_2_multiple(input, len, 16, &padded_len);
    if (!padded) return NULL;
    const uint8_t iv1[16] = {0};
    uint8_t* stage1 = s_malloc(16 + padded_len);
    memcpy(stage1, iv1, 16);
    cbc_encrypt(padded, stage1 + 16, padded_len, ctx->key1, iv1);
    const uint8_t iv2[16] = {0};
    const size_t stage1_len = 16 + padded_len;
    uint8_t* stage2 = s_malloc(16 + stage1_len);
    memcpy(stage2, iv2, 16);
    cbc_encrypt(stage1, stage2 + 16, stage1_len, ctx->key2, iv2);
    char* hex = bytes_2_hex(stage2, 16 + stage1_len);
    s_free(padded);
    s_free(stage1);
    s_free(stage2);
    return hex;
}

static char* aes_cbc_decrypt(cipher_interface_t* self, const char* hex)
{
    if (!self || !hex) return NULL;
    const aes_cbc_linux_ctx_t* ctx = self->private_data;
    size_t in_len = 0;
    uint8_t* in = hex_2_bytes(hex, &in_len);
    if (!in || in_len < 32 || (in_len % 16)!=0)
    {
        s_free(in);
        return NULL;
    }
    const uint8_t* iv2 = in;
    const uint8_t* c2 = in + 16;
    const size_t c2_len = in_len - 16;
    uint8_t* stage1 = s_malloc(c2_len);
    cbc_decrypt(c2, stage1, c2_len, ctx->key2, iv2);
    s_free(in);
    if (c2_len < 32)
    {
        s_free(stage1);
        return NULL;
    }
    const uint8_t* iv1 = stage1;
    const uint8_t* c1 = stage1 + 16;
    const size_t c1_len = c2_len - 16;
    uint8_t* out = s_malloc(c1_len);
    cbc_decrypt(c1, out, c1_len, ctx->key1, iv1);
    s_free(stage1);
    size_t plain_len = c1_len;
    while (plain_len > 0 && out[plain_len - 1] == 0x00) plain_len--;
    char* text = s_malloc(plain_len + 1);
    memcpy(text, out, plain_len);
    text[plain_len] = '\0';
    s_free(out);
    return text;
}

static void aes_cbc_destroy(cipher_interface_t* self)
{
    if (!self) return;
    if (self->private_data) s_free(self->private_data);
    s_free(self);
}

cipher_interface_t* create_aes_cbc_linux_cipher(const uint8_t* key1, const uint8_t* key2)
{
    if (!key1 || !key2) return NULL;
    cipher_interface_t* ci = s_calloc(1, sizeof(cipher_interface_t));
    aes_cbc_linux_ctx_t* ctx = s_calloc(1, sizeof(aes_cbc_linux_ctx_t));
    memcpy(ctx->key1, key1, 16);
    memcpy(ctx->key2, key2, 16);
    ci->encrypt = aes_cbc_encrypt;
    ci->decrypt = aes_cbc_decrypt;
    ci->destroy = aes_cbc_destroy;
    ci->private_data = ctx;
    return ci;
}