#include "cipher/CipherInterface.h"
#include "cipher/CipherUtils.h"

#include <string.h>
#include <stdint.h>
#include "xtea_common.h"

typedef struct {
    uint32_t key1[4];
    uint32_t key2[4];
    uint32_t key3[4];
    bool swap_key_endian;
} mod_xtea_data_t;

static char* mod_xtea_encrypt(cipher_interface_t* self, const char* text)
{
    if (!self || !text) return NULL;
    mod_xtea_data_t* data = self->private_data;
    if (!data) return NULL;
    const size_t text_len = strlen(text);
    size_t padded_len;
    uint8_t* padded = pad_2_multiple((const uint8_t*)text, text_len, 8, &padded_len);
    if (!padded) return NULL;
    uint8_t* output = s_malloc(padded_len);
    memcpy(output, padded, padded_len);
    s_free(padded);
    for (size_t i = 0; i < padded_len; i += 8)
    {
        uint32_t v0 = xtea_get_be32(output, i);
        uint32_t v1 = xtea_get_be32(output, i + 4);
        xtea_encrypt_block(&v0, &v1, data->key1, data->swap_key_endian);
        xtea_encrypt_block(&v0, &v1, data->key2, data->swap_key_endian);
        xtea_encrypt_block(&v0, &v1, data->key3, data->swap_key_endian);
        xtea_set_be32(output, i, v0);
        xtea_set_be32(output, i + 4, v1);
    }
    char* hex = bytes_2_hex(output, padded_len);
    s_free(output);
    return hex;
}

static char* mod_xtea_decrypt(cipher_interface_t* self, const char* hex)
{
    if (!self || !hex) return NULL;
    mod_xtea_data_t* data = self->private_data;
    if (!data) return NULL;
    size_t bytes_len;
    uint8_t* bytes = hex_2_bytes(hex, &bytes_len);
    if (!bytes) return NULL;
    uint8_t* output = s_malloc(bytes_len);
    memcpy(output, bytes, bytes_len);
    s_free(bytes);
    for (size_t i = 0; i < bytes_len; i += 8)
    {
        uint32_t v0 = xtea_get_be32(output, i);
        uint32_t v1 = xtea_get_be32(output, i + 4);
        xtea_decrypt_block(&v0, &v1, data->key3, data->swap_key_endian);
        xtea_decrypt_block(&v0, &v1, data->key2, data->swap_key_endian);
        xtea_decrypt_block(&v0, &v1, data->key1, data->swap_key_endian);
        xtea_set_be32(output, i, v0);
        xtea_set_be32(output, i + 4, v1);
    }
    while (bytes_len > 0 && output[bytes_len - 1] == 0) bytes_len--;
    char* result = s_malloc(bytes_len + 1);
    memcpy(result, output, bytes_len);
    result[bytes_len] = '\0';
    s_free(output);
    return result;
}

static void mod_xtea_destroy(cipher_interface_t* self)
{
    if (self)
    {
        s_free(self->private_data);
        s_free(self);
    }
}

cipher_interface_t* create_mod_xtea_cipher(const uint32_t* key1, const uint32_t* key2,
                                              const uint32_t* key3, const bool swap_key_endian)
{
    if (!key1 || !key2 || !key3) return NULL;
    cipher_interface_t* cipher = s_malloc(sizeof(cipher_interface_t));
    mod_xtea_data_t* data = s_malloc(sizeof(mod_xtea_data_t));
    memcpy(data->key1, key1, 4 * sizeof(uint32_t));
    memcpy(data->key2, key2, 4 * sizeof(uint32_t));
    memcpy(data->key3, key3, 4 * sizeof(uint32_t));
    data->swap_key_endian = swap_key_endian;
    cipher->encrypt = mod_xtea_encrypt;
    cipher->decrypt = mod_xtea_decrypt;
    cipher->destroy = mod_xtea_destroy;
    cipher->private_data = data;
    return cipher;
}