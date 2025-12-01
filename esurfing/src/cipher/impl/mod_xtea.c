#include "../../headFiles/cipher/CipherInterface.h"
#include "../../headFiles/cipher/CipherUtils.h"
#include <string.h>
#include <stdint.h>

#define XTEA_NUM_ROUNDS 32
#define XTEA_DELTA 0x9E3779B9

typedef struct {
    uint32_t key1[4];
    uint32_t key2[4];
    uint32_t key3[4];
} mod_xtea_data_t;

static uint32_t get_uint32_be(const uint8_t* data, const size_t offset)
{
    return data[offset] << 24 | data[offset + 1] << 16 |
           data[offset + 2] << 8 | data[offset + 3];
}

static void set_uint32_be(uint8_t* data, const size_t offset, const uint32_t value)
{
    data[offset] = (value >> 24) & 0xFF;
    data[offset + 1] = (value >> 16) & 0xFF;
    data[offset + 2] = (value >> 8) & 0xFF;
    data[offset + 3] = value & 0xFF;
}

static void xtea_encrypt_block(uint32_t* v0, uint32_t* v1, const uint32_t* key)
{
    uint32_t sum = 0;
    for (int i = 0; i < XTEA_NUM_ROUNDS; i++)
    {
        *v0 += (*v1 ^ sum) + key[sum & 3] + (*v1 << 4 ^ *v1 >> 5);
        sum += XTEA_DELTA;
        *v1 += key[sum >> 11 & 3] + (*v0 ^ sum) + (*v0 << 4 ^ *v0 >> 5);
    }
}

static void xtea_decrypt_block(uint32_t* v0, uint32_t* v1, const uint32_t* key)
{
    uint32_t sum = XTEA_DELTA * XTEA_NUM_ROUNDS;
    for (int i = 0; i < XTEA_NUM_ROUNDS; i++)
    {
        *v1 -= key[sum >> 11 & 3] + (*v0 ^ sum) + (*v0 << 4 ^ (*v0 >> 5));
        sum -= XTEA_DELTA;
        *v0 -= (*v1 ^ sum) + key[sum & 3] + (*v1 << 4 ^ *v1 >> 5);
    }
}

static char* mod_xtea_encrypt(cipherInterfaceT* self, const char* text)
{
    if (!self || !text) return NULL;
    mod_xtea_data_t* data = self->private_data;
    if (!data) return NULL;
    const size_t text_len = strlen(text);
    size_t padded_len;
    uint8_t* padded_data = padToMultiple((const uint8_t*)text, text_len, 8, &padded_len);
    if (!padded_data) return NULL;
    uint8_t* output = safeMalloc(padded_len);
    memcpy(output, padded_data, padded_len);
    safeFree(padded_data);
    for (size_t i = 0; i < padded_len; i += 8)
    {
        uint32_t v0 = get_uint32_be(output, i);
        uint32_t v1 = get_uint32_be(output, i + 4);
        xtea_encrypt_block(&v0, &v1, data->key1);
        xtea_encrypt_block(&v0, &v1, data->key2);
        xtea_encrypt_block(&v0, &v1, data->key3);
        set_uint32_be(output, i, v0);
        set_uint32_be(output, i + 4, v1);
    }
    char* hex_result = bytesToHexUpper(output, padded_len);
    safeFree(output);
    return hex_result;
}

static char* mod_xtea_decrypt(cipherInterfaceT* self, const char* hex)
{
    if (!self || !hex) return NULL;
    mod_xtea_data_t* data = self->private_data;
    if (!data) return NULL;
    size_t bytes_len;
    uint8_t* bytes = hexToBytes(hex, &bytes_len);
    if (!bytes) return NULL;
    uint8_t* output = safeMalloc(bytes_len);
    memcpy(output, bytes, bytes_len);
    safeFree(bytes);
    for (size_t i = 0; i < bytes_len; i += 8)
    {
        uint32_t v0 = get_uint32_be(output, i);
        uint32_t v1 = get_uint32_be(output, i + 4);
        xtea_decrypt_block(&v0, &v1, data->key3);
        xtea_decrypt_block(&v0, &v1, data->key2);
        xtea_decrypt_block(&v0, &v1, data->key1);
        set_uint32_be(output, i, v0);
        set_uint32_be(output, i + 4, v1);
    }
    while (bytes_len > 0 && output[bytes_len - 1] == 0)
    {
        bytes_len--;
    }
    char* result = safeMalloc(bytes_len + 1);
    memcpy(result, output, bytes_len);
    result[bytes_len] = '\0';
    safeFree(output);
    return result;
}

static void mod_xtea_destroy(cipherInterfaceT* self)
{
    if (self)
    {
        safeFree(self->private_data);
        safeFree(self);
    }
}

cipherInterfaceT* create_mod_xtea_cipher(const uint32_t* key1, const uint32_t* key2,
                                           const uint32_t* key3)
{
    if (!key1 || !key2 || !key3) return NULL;
    cipherInterfaceT* cipher = safeMalloc(sizeof(cipherInterfaceT));
    mod_xtea_data_t* data = safeMalloc(sizeof(mod_xtea_data_t));
    memcpy(data->key1, key1, 4 * sizeof(uint32_t));
    memcpy(data->key2, key2, 4 * sizeof(uint32_t));
    memcpy(data->key3, key3, 4 * sizeof(uint32_t));
    cipher->encrypt = mod_xtea_encrypt;
    cipher->decrypt = mod_xtea_decrypt;
    cipher->destroy = mod_xtea_destroy;
    cipher->private_data = data;
    return cipher;
}