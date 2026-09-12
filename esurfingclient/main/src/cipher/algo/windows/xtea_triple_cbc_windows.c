#include "cipher/CipherInterface.h"
#include "cipher/CipherUtils.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define XTEA_DELTA 0x9E3779B9u
#define XTEA_TRIPLE_CBC_KEY_SIZE 48
#define XTEA_TRIPLE_CBC_IV_SIZE 8

typedef struct
{
    uint8_t key[XTEA_TRIPLE_CBC_KEY_SIZE];
    uint8_t iv[XTEA_TRIPLE_CBC_IV_SIZE];
} xtea_triple_cbc_windows_t;

static uint32_t read_u32_be(const uint8_t *data)
{
    return ((uint32_t)data[0] << 24) |
           ((uint32_t)data[1] << 16) |
           ((uint32_t)data[2] << 8) |
           ((uint32_t)data[3]);
}

static void write_u32_be(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)(value >> 24);
    data[1] = (uint8_t)(value >> 16);
    data[2] = (uint8_t)(value >> 8);
    data[3] = (uint8_t)value;
}

static void xor_block(
    const uint8_t a[8],
    const uint8_t b[8],
    uint8_t result[8])
{
    for (int i = 0; i < 8; i++)
    {
        result[i] = a[i] ^ b[i];
    }
}

static uint32_t xtea_f(uint32_t x)
{
    return (x << 4) ^ (x >> 5);
}

static void xtea_block(
    const uint8_t block[8],
    const uint8_t key[16],
    int rounds,
    uint8_t result[8])
{
    uint32_t v0 = read_u32_be(block);
    uint32_t v1 = read_u32_be(block + 4);

    uint32_t k[4];

    k[0] = read_u32_be(key);
    k[1] = read_u32_be(key + 4);
    k[2] = read_u32_be(key + 8);
    k[3] = read_u32_be(key + 12);

    if (rounds > 0)
    {
        uint32_t sum = 0;

        for (int i = 0; i < rounds; i++)
        {
            v0 += xtea_f(v1) + k[sum & 3] + (sum ^ v1);
            sum += XTEA_DELTA;
            v1 += xtea_f(v0) + k[(sum >> 11) & 3] + (sum ^ v0);
        }
    }
    else
    {
        int n = -rounds;

        uint32_t sum = (uint32_t)n * XTEA_DELTA;

        for (int i = 0; i < n; i++)
        {
            v1 -= xtea_f(v0) + k[(sum >> 11) & 3] + (sum ^ v0);
            sum -= XTEA_DELTA;
            v0 -= xtea_f(v1) + k[sum & 3] + (sum ^ v1);
        }
    }

    write_u32_be(result, v0);
    write_u32_be(result + 4, v1);
}

static char *xtea_triple_cbc_windows_encrypt(cipher_interface_t *self, const char *plaintext)
{
    if (!self || !plaintext)
    {
        return NULL;
    }

    const xtea_triple_cbc_windows_t *d = (const xtea_triple_cbc_windows_t *)self->private_data;

    if (!d)
    {
        return NULL;
    }

    const size_t plaintext_len = strlen(plaintext);
    const size_t padded_len = (plaintext_len + 7) & ~(size_t)7;

    if (padded_len == 0)
    {
        char *hex = (char *)malloc(1);

        if (!hex)
        {
            return NULL;
        }

        hex[0] = '\0';
        return hex;
    }

    uint8_t *data = (uint8_t *)calloc(1, padded_len);

    if (!data)
    {
        return NULL;
    }
    memcpy(data, plaintext, plaintext_len);

    uint8_t prev[8];
    memcpy(prev, d->iv, sizeof(prev));

    for (size_t offset = 0; offset < padded_len; offset += 8)
    {
        uint8_t block[8];
        uint8_t tmp[8];

        memcpy(block, data + offset, sizeof(block));

        xor_block(block, prev, tmp);

        xtea_block(tmp, d->key, 32, block);
        xtea_block(block, d->key + 16, 32, tmp);
        xtea_block(tmp, d->key + 32, 32, block);

        memcpy(data + offset, block, sizeof(block));

        memcpy(prev, block, sizeof(prev));
    }

    char *hex = bytes_2_hex(data, padded_len);

    s_free(data);

    return hex;
}

static char *xtea_triple_cbc_windows_decrypt(cipher_interface_t *self, const char *hex)
{
    const xtea_triple_cbc_windows_t *d =
        (const xtea_triple_cbc_windows_t *)self->private_data;

    size_t bytes_len = 0;

    uint8_t *bytes = hex_2_bytes(hex, &bytes_len);

    if (!bytes && bytes_len != 0)
    {
        return NULL;
    }

    if (bytes_len % 8 != 0)
    {
        free(bytes);
        return NULL;
    }

    uint8_t *out = (uint8_t *)malloc(bytes_len + 1);

    if (!out)
    {
        free(bytes);
        return NULL;
    }

    uint8_t prev[8];
    memcpy(prev, d->iv, 8);

    for (size_t offset = 0; offset < bytes_len; offset += 8)
    {
        uint8_t block[8];
        uint8_t tmp[8];
        uint8_t tmp2[8];
        uint8_t plain[8];

        memcpy(block, bytes + offset, 8);
        xtea_block(block, d->key + 32, -32, tmp);
        xtea_block(tmp, d->key + 16, -32, tmp2);
        xtea_block(tmp2, d->key, -32, tmp);
        xor_block(tmp, prev, plain);
        memcpy(out + offset, plain, 8);
        memcpy(prev, block, 8);
    }

    free(bytes);

    size_t end = bytes_len;

    for (size_t i = 0; i < bytes_len; i++)
    {
        if (out[i] == 0)
        {
            end = i;
            break;
        }
    }

    out[end] = '\0';

    return (char *)out;
}

static void xtea_triple_cbc_windows_destroy(cipher_interface_t* self)
{
    if(self)
    {
        s_free(self->private_data);
        s_free(self);
    }
}

cipher_interface_t *create_xtea_triple_cbc_windows_cipher(const uint8_t *key, const uint8_t *iv)
{
    if (!key || !iv)
    {
        return NULL;
    }
    cipher_interface_t *ci = s_calloc(1, sizeof(cipher_interface_t));
    xtea_triple_cbc_windows_t *d = s_malloc(sizeof(xtea_triple_cbc_windows_t));
    memcpy(d->key, key, XTEA_TRIPLE_CBC_KEY_SIZE);
    memcpy(d->iv, iv, XTEA_TRIPLE_CBC_IV_SIZE);
    ci->encrypt = xtea_triple_cbc_windows_encrypt;
    ci->decrypt = xtea_triple_cbc_windows_decrypt;
    ci->destroy = xtea_triple_cbc_windows_destroy;
    ci->private_data = d;
    return ci;
}
