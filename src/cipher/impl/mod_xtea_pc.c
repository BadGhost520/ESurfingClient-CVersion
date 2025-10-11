#include "../../headFiles/cipher/cipher_interface.h"
#include "../../headFiles/cipher/cipher_utils.h"
#include <string.h>
#include <stdint.h>

#define XTEA_NUM_ROUNDS 32
#define XTEA_DELTA 0x9E3779B9u

typedef struct {
    uint32_t key1[4];
    uint32_t key2[4];
    uint32_t key3[4];
} mod_xtea_pc_data_t;

// BE helpers
static uint32_t get_uint32_be(const uint8_t* data, size_t offset) {
    return (uint32_t)data[offset] << 24 | (uint32_t)data[offset + 1] << 16 |
           (uint32_t)data[offset + 2] << 8 | (uint32_t)data[offset + 3];
}
static void set_uint32_be(uint8_t* data, size_t offset, uint32_t value) {
    data[offset] = (uint8_t)((value >> 24) & 0xFF);
    data[offset + 1] = (uint8_t)((value >> 16) & 0xFF);
    data[offset + 2] = (uint8_t)((value >> 8) & 0xFF);
    data[offset + 3] = (uint8_t)(value & 0xFF);
}

// XTEA encrypt block (matching decryption inverse of our PC variant)
static inline uint32_t bswap32(uint32_t x) {
    return (x >> 24) | ((x >> 8) & 0x0000FF00u) | ((x << 8) & 0x00FF0000u) | (x << 24);
}

static void xtea_encrypt_block_pc(uint32_t* v0, uint32_t* v1, const uint32_t* key) {
    uint32_t sum = 0;
    uint32_t ks[4] = { bswap32(key[0]), bswap32(key[1]), bswap32(key[2]), bswap32(key[3]) };
    for (int i = 0; i < XTEA_NUM_ROUNDS; ++i) {
        uint32_t kB = ks[sum & 3u];
        *v0 += ((*v1 << 4) ^ (*v1 >> 5)) + (sum ^ *v1) + kB;
        sum += XTEA_DELTA;
        uint32_t kA = ks[(sum >> 11) & 3u];
        *v1 += ((*v0 << 4) ^ (*v0 >> 5)) + (*v0 ^ sum) + kA;
    }
}

// XTEA decrypt block (PC variant, matches sub_50D0 a3<=0 half-round order)
static void xtea_decrypt_block_pc(uint32_t* v0, uint32_t* v1, const uint32_t* key) {
    uint32_t sum = XTEA_DELTA * XTEA_NUM_ROUNDS;
    uint32_t ks[4] = { bswap32(key[0]), bswap32(key[1]), bswap32(key[2]), bswap32(key[3]) };
    for (int i = 0; i < XTEA_NUM_ROUNDS; ++i) {
        uint32_t kA = ks[(sum >> 11) & 3u];
        *v1 -= ((*v0 << 4) ^ (*v0 >> 5)) + (*v0 ^ sum) + kA;
        sum -= XTEA_DELTA;
        uint32_t kB = ks[sum & 3u];
        *v0 -= ((*v1 << 4) ^ (*v1 >> 5)) + (sum ^ *v1) + kB;
    }
}

// encrypt
static char* mod_xtea_pc_encrypt(cipher_interface_t* self, const char* text) {
    if (!self || !text) return NULL;
    mod_xtea_pc_data_t* data = (mod_xtea_pc_data_t*)self->private_data;
    if (!data) return NULL;

    size_t text_len = strlen(text);
    size_t padded_len;
    uint8_t* padded = pad_to_multiple((const uint8_t*)text, text_len, 8, &padded_len);
    if (!padded) return NULL;
    uint8_t* output = safe_malloc(padded_len);
    memcpy(output, padded, padded_len);
    safe_free(padded);

    for (size_t i = 0; i < padded_len; i += 8) {
        uint32_t v0 = get_uint32_be(output, i);
        uint32_t v1 = get_uint32_be(output, i + 4);
        xtea_encrypt_block_pc(&v0, &v1, data->key1);
        xtea_encrypt_block_pc(&v0, &v1, data->key2);
        xtea_encrypt_block_pc(&v0, &v1, data->key3);
        set_uint32_be(output, i, v0);
        set_uint32_be(output, i + 4, v1);
    }

    char* hex = bytes_to_hex_upper(output, padded_len);
    safe_free(output);
    return hex;
}

// decrypt
static char* mod_xtea_pc_decrypt(cipher_interface_t* self, const char* hex) {
    if (!self || !hex) return NULL;
    mod_xtea_pc_data_t* data = (mod_xtea_pc_data_t*)self->private_data;
    if (!data) return NULL;

    size_t bytes_len;
    uint8_t* bytes = hex_to_bytes(hex, &bytes_len);
    if (!bytes) return NULL;
    uint8_t* output = safe_malloc(bytes_len);
    memcpy(output, bytes, bytes_len);
    safe_free(bytes);

    for (size_t i = 0; i < bytes_len; i += 8) {
        uint32_t v0 = get_uint32_be(output, i);
        uint32_t v1 = get_uint32_be(output, i + 4);
        xtea_decrypt_block_pc(&v0, &v1, data->key3);
        xtea_decrypt_block_pc(&v0, &v1, data->key2);
        xtea_decrypt_block_pc(&v0, &v1, data->key1);
        set_uint32_be(output, i, v0);
        set_uint32_be(output, i + 4, v1);
    }

    while (bytes_len > 0 && output[bytes_len - 1] == 0) bytes_len--;
    char* result = safe_malloc(bytes_len + 1);
    memcpy(result, output, bytes_len);
    result[bytes_len] = '\0';
    safe_free(output);
    return result;
}

static void mod_xtea_pc_destroy(cipher_interface_t* self) {
    if (self) {
        safe_free(self->private_data);
        safe_free(self);
    }
}

cipher_interface_t* create_mod_xtea_pc_cipher(const uint32_t* key1, const uint32_t* key2,
                                              const uint32_t* key3) {
    if (!key1 || !key2 || !key3) return NULL;
    cipher_interface_t* cipher = safe_malloc(sizeof(cipher_interface_t));
    mod_xtea_pc_data_t* data = safe_malloc(sizeof(mod_xtea_pc_data_t));
    memcpy(data->key1, key1, 4 * sizeof(uint32_t));
    memcpy(data->key2, key2, 4 * sizeof(uint32_t));
    memcpy(data->key3, key3, 4 * sizeof(uint32_t));
    cipher->encrypt = mod_xtea_pc_encrypt;
    cipher->decrypt = mod_xtea_pc_decrypt;
    cipher->destroy = mod_xtea_pc_destroy;
    cipher->private_data = data;
    return cipher;
}