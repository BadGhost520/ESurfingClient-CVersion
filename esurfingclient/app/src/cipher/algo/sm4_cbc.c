#include "cipher/CipherInterface.h"
#include "cipher/CipherUtils.h"
#include "sm4_common.h"

typedef struct {
    uint8_t key[SM4_KEY_SIZE];
    uint8_t iv[SM4_BLOCK_SIZE];
} sm4_cbc_data_t;

static uint8_t* sm4_encrypt_cbc(const uint8_t* key, const uint8_t* iv, const uint8_t* plaintext,
                                const size_t plaintext_len, size_t* output_len)
{
    size_t padded_len=0; uint8_t* padded = pkcs7_padding(plaintext, plaintext_len, SM4_BLOCK_SIZE, &padded_len);
    if(!padded) return NULL;
    uint8_t* out = s_malloc(padded_len);
    uint8_t prev[SM4_BLOCK_SIZE]; memcpy(prev, iv, SM4_BLOCK_SIZE);
    uint32_t rk[32]; sm4_key_expansion(key, rk);
    const size_t blocks = padded_len / SM4_BLOCK_SIZE;
    for(size_t i=0;i<blocks;i++)
    {
        uint8_t xored[SM4_BLOCK_SIZE];
        xor_bytes(padded + i*SM4_BLOCK_SIZE, prev, xored, SM4_BLOCK_SIZE);
        sm4_encrypt_block(xored, out + i*SM4_BLOCK_SIZE, rk);
        memcpy(prev, out + i*SM4_BLOCK_SIZE, SM4_BLOCK_SIZE);
    }
    s_free(padded);
    *output_len = padded_len;
    return out;
}

static uint8_t* sm4_decrypt_cbc(const uint8_t* key, const uint8_t* iv, const uint8_t* ciphertext,
                                const size_t ciphertext_len, size_t* output_len)
{
    if(ciphertext_len % SM4_BLOCK_SIZE != 0) return NULL;
    uint8_t* out_blocks = s_malloc(ciphertext_len);
    uint8_t prev[SM4_BLOCK_SIZE]; memcpy(prev, iv, SM4_BLOCK_SIZE);
    uint32_t rk[32]; sm4_key_expansion(key, rk);
    const size_t blocks = ciphertext_len / SM4_BLOCK_SIZE;
    for(size_t i=0;i<blocks;i++)
    {
        uint8_t dec[SM4_BLOCK_SIZE];
        const uint8_t* block = ciphertext + i*SM4_BLOCK_SIZE;
        sm4_decrypt_block(block, dec, rk);
        xor_bytes(dec, prev, out_blocks + i*SM4_BLOCK_SIZE, SM4_BLOCK_SIZE);
        memcpy(prev, block, SM4_BLOCK_SIZE);
    }
    size_t unpadded_len=0; uint8_t* unpadded = rm_pkcs7_padding(out_blocks, ciphertext_len, &unpadded_len);
    s_free(out_blocks);
    *output_len = unpadded_len;
    return unpadded;
}

static char* sm4_cbc_encrypt(cipher_interface_t* self, const char* text)
{
    if(!self || !text) return NULL;
    const sm4_cbc_data_t* d = self->private_data;
    const size_t text_len = strlen(text);
    size_t out_len=0; uint8_t* out = sm4_encrypt_cbc(d->key, d->iv, (const uint8_t*)text, text_len, &out_len);
    if(!out) return NULL;
    char* hex = bytes_2_hex(out, out_len);
    s_free(out);
    return hex;
}

static char* sm4_cbc_decrypt(cipher_interface_t* self, const char* hex)
{
    if(!self || !hex) return NULL;
    const sm4_cbc_data_t* d = self->private_data;
    size_t bytes_len=0; uint8_t* bytes = hex_2_bytes(hex, &bytes_len);
    if(!bytes) return NULL;
    size_t out_len=0; uint8_t* out = sm4_decrypt_cbc(d->key, d->iv, bytes, bytes_len, &out_len);
    s_free(bytes);
    if(!out) return NULL;
    char* result = s_malloc(out_len + 1);
    memcpy(result, out, out_len);
    result[out_len] = '\0';
    s_free(out);
    return result;
}

static void sm4_cbc_destroy(cipher_interface_t* self)
{
    if(self)
    {
        s_free(self->private_data); s_free(self);
    }
}

cipher_interface_t* create_sm4_cbc_cipher(const uint8_t* key, const uint8_t* iv)
{
    if(!key || !iv) return NULL;
    cipher_interface_t* ci = s_malloc(sizeof(cipher_interface_t));
    sm4_cbc_data_t* d = s_malloc(sizeof(sm4_cbc_data_t));
    memcpy(d->key, key, SM4_KEY_SIZE);
    memcpy(d->iv, iv, SM4_BLOCK_SIZE);
    ci->encrypt = sm4_cbc_encrypt;
    ci->decrypt = sm4_cbc_decrypt;
    ci->destroy = sm4_cbc_destroy;
    ci->private_data = d;
    return ci;
}