#include "cipher/CipherInterface.h"
#include "cipher/CipherUtils.h"

#include <openssl/des.h>
#include <openssl/evp.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    uint8_t key1[24];
    uint8_t key2[24];
} desede_ecb_data_t;

static uint8_t* desede_encrypt_ecb(const uint8_t* data, const size_t data_len,
                                   const uint8_t* key, size_t* out_len)
{
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return NULL;
    if (EVP_EncryptInit_ex(ctx, EVP_des_ede3_ecb(), NULL, key, NULL) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        return NULL;
    }
    EVP_CIPHER_CTX_set_padding(ctx, 0);
    size_t padded_len;
    uint8_t* padded_data = pad_2_multiple(data, data_len, 8, &padded_len);
    if (!padded_data)
    {
        EVP_CIPHER_CTX_free(ctx);
        return NULL;
    }
    uint8_t* output = s_malloc(padded_len);
    int len;
    int ciphertext_len = 0;
    if (EVP_EncryptUpdate(ctx, output, &len, padded_data, padded_len) != 1)
    {
        s_free(padded_data);
        s_free(output);
        EVP_CIPHER_CTX_free(ctx);
        return NULL;
    }
    ciphertext_len = len;
    if (EVP_EncryptFinal_ex(ctx, output + len, &len) != 1)
    {
        s_free(padded_data);
        s_free(output);
        EVP_CIPHER_CTX_free(ctx);
        return NULL;
    }
    ciphertext_len += len;
    s_free(padded_data);
    EVP_CIPHER_CTX_free(ctx);
    *out_len = ciphertext_len;
    return output;
}

static uint8_t* desede_decrypt_ecb(const uint8_t* data, const size_t data_len,
                                   const uint8_t* key, size_t* out_len)
{
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return NULL;
    if (EVP_DecryptInit_ex(ctx, EVP_des_ede3_ecb(), NULL, key, NULL) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        return NULL;
    }
    EVP_CIPHER_CTX_set_padding(ctx, 0);
    uint8_t* output = s_malloc(data_len);
    int len;
    int plaintext_len = 0;
    if (EVP_DecryptUpdate(ctx, output, &len, data, data_len) != 1)
    {
        s_free(output);
        EVP_CIPHER_CTX_free(ctx);
        return NULL;
    }
    plaintext_len = len;
    if (EVP_DecryptFinal_ex(ctx, output + len, &len) != 1)
    {
        s_free(output);
        EVP_CIPHER_CTX_free(ctx);
        return NULL;
    }
    plaintext_len += len;
    EVP_CIPHER_CTX_free(ctx);
    *out_len = plaintext_len;
    return output;
}

static char* desede_ecb_encrypt(cipher_interface_t* self, const char* text)
{
    if (!self || !text) return NULL;
    desede_ecb_data_t* data = self->private_data;
    if (!data) return NULL;
    const size_t text_len = strlen(text);
    size_t r1_len;
    uint8_t* r1 = desede_encrypt_ecb((const uint8_t*)text, text_len, 
                                     data->key1, &r1_len);
    if (!r1) return NULL;
    size_t r2_len;
    uint8_t* r2 = desede_encrypt_ecb(r1, r1_len, data->key2, &r2_len);
    s_free(r1);
    if (!r2) return NULL;
    char* hex_result = bytes_2_hex(r2, r2_len);
    s_free(r2);
    return hex_result;
}

static char* desede_ecb_decrypt(cipher_interface_t* self, const char* hex)
{
    if (!self || !hex) return NULL;
    desede_ecb_data_t* data = self->private_data;
    if (!data) return NULL;
    size_t bytes_len;
    uint8_t* bytes = hex_2_bytes(hex, &bytes_len);
    if (!bytes) return NULL;
    size_t r1_len;
    uint8_t* r1 = desede_decrypt_ecb(bytes, bytes_len, data->key2, &r1_len);
    s_free(bytes);
    if (!r1) return NULL;
    size_t r2_len;
    uint8_t* r2 = desede_decrypt_ecb(r1, r1_len, data->key1, &r2_len);
    s_free(r1);
    if (!r2) return NULL;
    while (r2_len > 0 && r2[r2_len - 1] == 0)
    {
        r2_len--;
    }
    char* result = s_malloc(r2_len + 1);
    memcpy(result, r2, r2_len);
    result[r2_len] = '\0';
    s_free(r2);
    return result;
}

// 销毁函数
static void desede_ecb_destroy(cipher_interface_t* self)
{
    if (self)
    {
        s_free(self->private_data);
        s_free(self);
    }
}

cipher_interface_t* create_desede_ecb_cipher(const uint8_t* key1, const uint8_t* key2)
{
    if (!key1 || !key2) return NULL;
    cipher_interface_t* cipher = s_malloc(sizeof(cipher_interface_t));
    desede_ecb_data_t* data = s_malloc(sizeof(desede_ecb_data_t));
    memcpy(data->key1, key1, 24);
    memcpy(data->key2, key2, 24);
    cipher->encrypt = desede_ecb_encrypt;
    cipher->decrypt = desede_ecb_decrypt;
    cipher->destroy = desede_ecb_destroy;
    cipher->private_data = data;
    return cipher;
}