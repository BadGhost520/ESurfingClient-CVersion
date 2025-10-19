#include "../../headFiles/cipher/CipherInterface.h"
#include "../../headFiles/cipher/CipherUtils.h"
#include <openssl/des.h>
#include <openssl/evp.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    uint8_t key1[24];
    uint8_t key2[24];
    uint8_t iv1[8];
    uint8_t iv2[8];
} desede_cbc_data_t;

static uint8_t* desede_encrypt_cbc(const uint8_t* data, size_t data_len, 
                                   const uint8_t* key, const uint8_t* iv, 
                                   size_t* out_len)
{
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return NULL;
    if (EVP_EncryptInit_ex(ctx, EVP_des_ede3_cbc(), NULL, key, iv) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        return NULL;
    }
    EVP_CIPHER_CTX_set_padding(ctx, 0);
    size_t padded_len;
    uint8_t* padded_data = padToMultiple(data, data_len, 8, &padded_len);
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

static uint8_t* desede_decrypt_cbc(const uint8_t* data, size_t data_len, 
                                   const uint8_t* key, const uint8_t* iv, 
                                   size_t* out_len)
{
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return NULL;
    if (EVP_DecryptInit_ex(ctx, EVP_des_ede3_cbc(), NULL, key, iv) != 1)
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

static char* desede_cbc_encrypt(cipher_interface_t* self, const char* text)
{
    if (!self || !text) return NULL;
    desede_cbc_data_t* data = self->private_data;
    if (!data) return NULL;
    const size_t text_len = strlen(text);
    size_t r1_len;
    uint8_t* r1 = desede_encrypt_cbc((const uint8_t*)text, text_len, 
                                     data->key1, data->iv1, &r1_len);
    if (!r1) return NULL;
    size_t r2_len;
    uint8_t* r2 = desede_encrypt_cbc(r1, r1_len, data->key2, data->iv2, &r2_len);
    safeFree(r1);
    if (!r2) return NULL;
    char* hex_result = bytesToHexUpper(r2, r2_len);
    safeFree(r2);
    return hex_result;
}

static char* desede_cbc_decrypt(cipher_interface_t* self, const char* hex)
{
    if (!self || !hex) return NULL;
    desede_cbc_data_t* data = self->private_data;
    if (!data) return NULL;
    size_t bytes_len;
    uint8_t* bytes = hexToBytes(hex, &bytes_len);
    if (!bytes) return NULL;
    size_t r1_len;
    uint8_t* r1 = desede_decrypt_cbc(bytes, bytes_len, data->key2, data->iv2, &r1_len);
    safeFree(bytes);
    if (!r1) return NULL;
    size_t r2_len;
    uint8_t* r2 = desede_decrypt_cbc(r1, r1_len, data->key1, data->iv1, &r2_len);
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

static void desede_cbc_destroy(cipher_interface_t* self)
{
    if (self)
    {
        safeFree(self->private_data);
        safeFree(self);
    }
}

cipher_interface_t* create_desede_cbc_cipher(const uint8_t* key1, const uint8_t* key2,
                                             const uint8_t* iv1, const uint8_t* iv2)
{
    if (!key1 || !key2 || !iv1 || !iv2) return NULL;
    cipher_interface_t* cipher = safeMalloc(sizeof(cipher_interface_t));
    desede_cbc_data_t* data = safeMalloc(sizeof(desede_cbc_data_t));
    memcpy(data->key1, key1, 24);
    memcpy(data->key2, key2, 24);
    memcpy(data->iv1, iv1, 8);
    memcpy(data->iv2, iv2, 8);
    cipher->encrypt = desede_cbc_encrypt;
    cipher->decrypt = desede_cbc_decrypt;
    cipher->destroy = desede_cbc_destroy;
    cipher->private_data = data;
    return cipher;
}