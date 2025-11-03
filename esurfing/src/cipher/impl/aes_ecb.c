#include "../../headFiles/cipher/CipherInterface.h"
#include "../../headFiles/cipher/CipherUtils.h"
#include <openssl/evp.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    uint8_t key1[16];
    uint8_t key2[16];
} aes_ecb_data_t;

static uint8_t* aes_encrypt_ecb(const uint8_t* data, const size_t data_len,
                                const uint8_t* key, size_t* out_len)
{
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return NULL;
    if (EVP_EncryptInit_ex(ctx, EVP_aes_128_ecb(), NULL, key, NULL) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        return NULL;
    }
    EVP_CIPHER_CTX_set_padding(ctx, 0);
    size_t padded_len;
    uint8_t* padded_data = padToMultiple(data, data_len, 16, &padded_len);
    if (!padded_data)
    {
        EVP_CIPHER_CTX_free(ctx);
        return NULL;
    }
    uint8_t* output = safeMalloc(padded_len);
    int len;
    int ciphertext_len = 0;
    if (EVP_EncryptUpdate(ctx, output, &len, padded_data, padded_len) != 1)
    {
        safeFree(padded_data);
        safeFree(output);
        EVP_CIPHER_CTX_free(ctx);
        return NULL;
    }
    ciphertext_len = len;
    if (EVP_EncryptFinal_ex(ctx, output + len, &len) != 1)
    {
        safeFree(padded_data);
        safeFree(output);
        EVP_CIPHER_CTX_free(ctx);
        return NULL;
    }
    ciphertext_len += len;
    safeFree(padded_data);
    EVP_CIPHER_CTX_free(ctx);
    *out_len = ciphertext_len;
    return output;
}

static uint8_t* aes_decrypt_ecb(const uint8_t* data, const size_t data_len,
                                const uint8_t* key, size_t* out_len)
{
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return NULL;
    if (EVP_DecryptInit_ex(ctx, EVP_aes_128_ecb(), NULL, key, NULL) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        return NULL;
    }
    EVP_CIPHER_CTX_set_padding(ctx, 0);
    uint8_t* output = safeMalloc(data_len);
    int len;
    int plaintext_len = 0;
    if (EVP_DecryptUpdate(ctx, output, &len, data, data_len) != 1)
    {
        safeFree(output);
        EVP_CIPHER_CTX_free(ctx);
        return NULL;
    }
    plaintext_len = len;
    if (EVP_DecryptFinal_ex(ctx, output + len, &len) != 1)
    {
        safeFree(output);
        EVP_CIPHER_CTX_free(ctx);
        return NULL;
    }
    plaintext_len += len;
    EVP_CIPHER_CTX_free(ctx);
    *out_len = plaintext_len;
    return output;
}

static char* aes_ecb_encrypt(cipher_interface_t* self, const char* text)
{
    if (!self || !text) return NULL;
    aes_ecb_data_t* data = self->private_data;
    if (!data) return NULL;
    const size_t text_len = strlen(text);
    size_t r1_len;
    uint8_t* r1 = aes_encrypt_ecb((const uint8_t*)text, text_len, 
                                  data->key1, &r1_len);
    if (!r1) return NULL;
    size_t r2_len;
    uint8_t* r2 = aes_encrypt_ecb(r1, r1_len, data->key2, &r2_len);
    safeFree(r1);
    if (!r2) return NULL;
    char* hex_result = bytesToHexUpper(r2, r2_len);
    safeFree(r2);
    return hex_result;
}

static char* aes_ecb_decrypt(cipher_interface_t* self, const char* hex)
{
    if (!self || !hex) return NULL;
    aes_ecb_data_t* data = self->private_data;
    if (!data) return NULL;
    size_t bytes_len;
    uint8_t* bytes = hexToBytes(hex, &bytes_len);
    if (!bytes) return NULL;
    size_t r1_len;
    uint8_t* r1 = aes_decrypt_ecb(bytes, bytes_len, data->key2, &r1_len);
    safeFree(bytes);
    if (!r1) return NULL;
    size_t r2_len;
    uint8_t* r2 = aes_decrypt_ecb(r1, r1_len, data->key1, &r2_len);
    safeFree(r1);
    if (!r2) return NULL;
    while (r2_len > 0 && r2[r2_len - 1] == 0)
    {
        r2_len--;
    }
    char* result = safeMalloc(r2_len + 1);
    memcpy(result, r2, r2_len);
    result[r2_len] = '\0';
    safeFree(r2);
    return result;
}

static void aes_ecb_destroy(cipher_interface_t* self)
{
    if (self)
    {
        safeFree(self->private_data);
        safeFree(self);
    }
}

cipher_interface_t* create_aes_ecb_cipher(const uint8_t* key1, const uint8_t* key2)
{
    if (!key1 || !key2) return NULL;
    cipher_interface_t* cipher = safeMalloc(sizeof(cipher_interface_t));
    aes_ecb_data_t* data = safeMalloc(sizeof(aes_ecb_data_t));
    memcpy(data->key1, key1, 16);
    memcpy(data->key2, key2, 16);
    cipher->encrypt = aes_ecb_encrypt;
    cipher->decrypt = aes_ecb_decrypt;
    cipher->destroy = aes_ecb_destroy;
    cipher->private_data = data;
    return cipher;
}