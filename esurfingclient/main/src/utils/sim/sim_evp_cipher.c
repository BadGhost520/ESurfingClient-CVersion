#include "utils/sim/sim_evp_internal.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct myssl_cipher_ctx_st
{
    const EVP_CIPHER* cipher;
    int enc;
    int padding;
    uint8_t key[24];
    uint8_t iv[16];
    uint8_t chain[16];
    uint8_t buf[16];
    size_t buflen;
    uint8_t hold[16];
    int have_hold;
    uint8_t rk[176];
    uint64_t dk[3][16];
};

/* ---------------------------- 对称密码 ---------------------------- */

EVP_CIPHER_CTX* EVP_CIPHER_CTX_new(void)
{
    EVP_CIPHER_CTX* ctx = (EVP_CIPHER_CTX*)malloc(sizeof(EVP_CIPHER_CTX));
    if (ctx) memset(ctx, 0, sizeof(*ctx));
    return ctx;
}

void EVP_CIPHER_CTX_free(EVP_CIPHER_CTX* ctx)
{
    if (ctx) free(ctx);
}

int EVP_CIPHER_CTX_set_padding(EVP_CIPHER_CTX* ctx, int padding)
{
    if (!ctx) return 0;
    ctx->padding = padding ? 1 : 0;
    return 1;
}

static int simssl_cipher_init(EVP_CIPHER_CTX* ctx, const EVP_CIPHER* type,
                             const unsigned char* key, const unsigned char* iv, int enc)
{
    if (!ctx || !type) return 0;

    ctx->cipher = type;
    ctx->enc = enc;
    ctx->buflen = 0;
    ctx->have_hold = 0;
    /* 与 OpenSSL 相同：默认开启 padding，需显式 set_padding(ctx, 0) 关闭 */
    ctx->padding = 1;

    if (key)
    {
        memcpy(ctx->key, key, (size_t)type->key_len);
        if (type->alg == MYSSL_ALG_AES_128_ECB || type->alg == MYSSL_ALG_AES_128_CBC)
        {
            aes128_expand_key(ctx->key, ctx->rk);
        }
        else
        {
            des_key_schedule(ctx->key, ctx->dk[0]);
            des_key_schedule(ctx->key + 8, ctx->dk[1]);
            des_key_schedule(ctx->key + 16, ctx->dk[2]);
        }
    }

    if (type->iv_len > 0 && iv) memcpy(ctx->iv, iv, (size_t)type->iv_len);
    memset(ctx->chain, 0, sizeof(ctx->chain));
    if (type->iv_len > 0) memcpy(ctx->chain, ctx->iv, (size_t)type->iv_len);
    return 1;
}

int EVP_EncryptInit_ex(EVP_CIPHER_CTX* ctx, const EVP_CIPHER* type, void* impl,
                       const unsigned char* key, const unsigned char* iv)
{
    (void)impl;
    return simssl_cipher_init(ctx, type, key, iv, 1);
}

int EVP_DecryptInit_ex(EVP_CIPHER_CTX* ctx, const EVP_CIPHER* type, void* impl,
                       const unsigned char* key, const unsigned char* iv)
{
    (void)impl;
    return simssl_cipher_init(ctx, type, key, iv, 0);
}

/* 单块处理，含 ECB/CBC 模式与链值维护 */
static void simssl_crypt_block(EVP_CIPHER_CTX* ctx, const uint8_t* in, uint8_t* out)
{
    const int bs = ctx->cipher->block_size;
    int is_des = (ctx->cipher->alg == MYSSL_ALG_DES_EDE3_ECB ||
                  ctx->cipher->alg == MYSSL_ALG_DES_EDE3_CBC);
    int is_cbc = (ctx->cipher->alg == MYSSL_ALG_AES_128_CBC ||
                  ctx->cipher->alg == MYSSL_ALG_DES_EDE3_CBC);
    uint8_t tmp[16];
    int i;

    if (is_cbc)
    {
        if (ctx->enc)
        {
            /* CBC 加密：C = E(P xor chain)，链值更新为密文块 */
            for (i = 0; i < bs; i++) tmp[i] = (uint8_t)(in[i] ^ ctx->chain[i]);
            if (is_des) simssl_des3_encrypt_block(ctx->key, tmp, out);
            else aes128_encrypt_block_rk(ctx->rk, tmp, out);
            memcpy(ctx->chain, out, (size_t)bs);
        }
        else
        {
            /* CBC 解密：P = D(C) xor chain，链值更新为密文块 */
            if (is_des) simssl_des3_decrypt_block(ctx->key, in, tmp);
            else aes128_decrypt_block_rk(ctx->rk, in, tmp);
            for (i = 0; i < bs; i++) out[i] = (uint8_t)(tmp[i] ^ ctx->chain[i]);
            memcpy(ctx->chain, in, (size_t)bs);
        }
        return;
    }

    if (is_des)
    {
        if (ctx->enc) simssl_des3_encrypt_block(ctx->key, in, out);
        else simssl_des3_decrypt_block(ctx->key, in, out);
    }
    else
    {
        if (ctx->enc) aes128_encrypt_block_rk(ctx->rk, in, out);
        else aes128_decrypt_block_rk(ctx->rk, in, out);
    }
}

static int simssl_update(EVP_CIPHER_CTX* ctx, unsigned char* out, int* outl,
                        const unsigned char* in, int inl)
{
    const int bs = ctx->cipher->block_size;
    int produced = 0;
    const uint8_t* p = in;
    int left = inl;

    if (!ctx || !outl || inl < 0) return 0;
    if (inl > 0 && !in) return 0;
    *outl = 0;

    while (left > 0)
    {
        size_t take = (size_t)bs - ctx->buflen;
        if (take > (size_t)left) take = (size_t)left;
        memcpy(ctx->buf + ctx->buflen, p, take);
        ctx->buflen += take;
        p += take;
        left -= (int)take;

        if (ctx->buflen == (size_t)bs)
        {
            if (!ctx->enc && ctx->padding)
            {
                /* 解密且开启 padding：延迟一块，供 Final 去填充 */
                if (ctx->have_hold)
                {
                    memcpy(out + produced, ctx->hold, (size_t)bs);
                    produced += bs;
                }
                simssl_crypt_block(ctx, ctx->buf, ctx->hold);
                ctx->have_hold = 1;
            }
            else
            {
                simssl_crypt_block(ctx, ctx->buf, out + produced);
                produced += bs;
            }
            ctx->buflen = 0;
        }
    }

    *outl = produced;
    return 1;
}

int EVP_EncryptUpdate(EVP_CIPHER_CTX* ctx, unsigned char* out, int* outl,
                      const unsigned char* in, int inl)
{
    if (!ctx || !ctx->cipher) return 0;
    return simssl_update(ctx, out, outl, in, inl);
}

int EVP_DecryptUpdate(EVP_CIPHER_CTX* ctx, unsigned char* out, int* outl,
                      const unsigned char* in, int inl)
{
    if (!ctx || !ctx->cipher) return 0;
    return simssl_update(ctx, out, outl, in, inl);
}

int EVP_EncryptFinal_ex(EVP_CIPHER_CTX* ctx, unsigned char* out, int* outl)
{
    const int bs = ctx->cipher ? ctx->cipher->block_size : 0;
    if (!ctx || !ctx->cipher || !outl) return 0;
    *outl = 0;

    if (!ctx->padding)
    {
        /* 关闭 padding 时，残留不足一块的数据视为错误（与 OpenSSL 一致） */
        return ctx->buflen == 0 ? 1 : 0;
    }

    {
        uint8_t pad = (uint8_t)(bs - (int)ctx->buflen);
        memset(ctx->buf + ctx->buflen, pad, pad);
        ctx->buflen = 0;
        simssl_crypt_block(ctx, ctx->buf, out);
        *outl = bs;
    }
    return 1;
}

int EVP_DecryptFinal_ex(EVP_CIPHER_CTX* ctx, unsigned char* out, int* outl)
{
    const int bs = ctx->cipher ? ctx->cipher->block_size : 0;
    if (!ctx || !ctx->cipher || !outl) return 0;
    *outl = 0;

    if (!ctx->padding)
    {
        return ctx->buflen == 0 ? 1 : 0;
    }

    if (ctx->buflen != 0 || !ctx->have_hold) return 0;

    {
        uint8_t pad = ctx->hold[bs - 1];
        int i;
        if (pad == 0 || pad > bs) return 0;
        for (i = bs - pad; i < bs; i++)
        {
            if (ctx->hold[i] != pad) return 0;
        }
        memcpy(out, ctx->hold, (size_t)(bs - pad));
        *outl = bs - pad;
        ctx->have_hold = 0;
    }
    return 1;
}
