#include "cipher/CipherInterface.h"
#include "cipher/CipherUtils.h"
#include "sm4_common.h"

typedef struct {
    uint8_t key[SM4_KEY_SIZE];
} sm4_ecb_data_t;

static uint8_t* sm4_encrypt_ecb(const uint8_t* key, const uint8_t* plaintext,
                                const size_t plaintext_len, size_t* output_len)
{
    size_t padded_len;
    uint8_t* padded_data = pkcs7_padding(plaintext, plaintext_len, SM4_BLOCK_SIZE, &padded_len);
    if (!padded_data) return NULL;
    const size_t blocks = padded_len / SM4_BLOCK_SIZE;
    uint8_t* output = s_malloc(padded_len);
    uint32_t rk[32];
    sm4_key_expansion(key, rk);
    for (size_t i = 0; i < blocks; i++)
    {
        sm4_encrypt_block(padded_data + i * SM4_BLOCK_SIZE, 
                          output + i * SM4_BLOCK_SIZE, rk);
    }
    s_free(padded_data);
    *output_len = padded_len;
    return output;
}

static uint8_t* sm4_decrypt_ecb(const uint8_t* key, const uint8_t* ciphertext,
                                const size_t ciphertext_len, size_t* output_len)
{
    if (ciphertext_len % SM4_BLOCK_SIZE != 0) return NULL;
    const size_t blocks = ciphertext_len / SM4_BLOCK_SIZE;
    uint8_t* output = s_malloc(ciphertext_len);
    uint32_t rk[32];
    sm4_key_expansion(key, rk);
    for (size_t i = 0; i < blocks; i++)
    {
        sm4_decrypt_block(ciphertext + i * SM4_BLOCK_SIZE, 
                          output + i * SM4_BLOCK_SIZE, rk);
    }
    size_t unpadded_len;
    uint8_t* unpadded_data = rm_pkcs7_padding(output, ciphertext_len, &unpadded_len);
    s_free(output);
    *output_len = unpadded_len;
    return unpadded_data;
}

static char* sm4_ecb_encrypt(cipher_interface_t* self, const char* text)
{
    if (!self || !text) return NULL;
    sm4_ecb_data_t* data = self->private_data;
    if (!data) return NULL;
    const size_t text_len = strlen(text);
    size_t output_len;
    uint8_t* encrypted = sm4_encrypt_ecb(data->key, (const uint8_t*)text, 
                                         text_len, &output_len);
    if (!encrypted) return NULL;
    char* hex_result = bytes_2_hex(encrypted, output_len);
    s_free(encrypted);
    return hex_result;
}

static char* sm4_ecb_decrypt(cipher_interface_t* self, const char* hex)
{
    if (!self || !hex) return NULL;
    sm4_ecb_data_t* data = self->private_data;
    if (!data) return NULL;
    size_t bytes_len;
    uint8_t* bytes = hex_2_bytes(hex, &bytes_len);
    if (!bytes) return NULL;
    size_t output_len;
    uint8_t* decrypted = sm4_decrypt_ecb(data->key, bytes, bytes_len, &output_len);
    s_free(bytes);
    if (!decrypted) return NULL;
    char* result = s_malloc(output_len + 1);
    memcpy(result, decrypted, output_len);
    result[output_len] = '\0';
    s_free(decrypted);
    return result;
}

static void sm4_ecb_destroy(cipher_interface_t* self)
{
    if (self)
    {
        s_free(self->private_data);
        s_free(self);
    }
}

cipher_interface_t* create_sm4_ecb_cipher(const uint8_t* key)
{
    if (!key) return NULL;
    cipher_interface_t* cipher = s_malloc(sizeof(cipher_interface_t));
    sm4_ecb_data_t* data = s_malloc(sizeof(sm4_ecb_data_t));
    memcpy(data->key, key, SM4_KEY_SIZE);
    cipher->encrypt = sm4_ecb_encrypt;
    cipher->decrypt = sm4_ecb_decrypt;
    cipher->destroy = sm4_ecb_destroy;
    cipher->private_data = data;
    return cipher;
}