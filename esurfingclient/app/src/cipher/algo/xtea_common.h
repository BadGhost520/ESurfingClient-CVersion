#ifndef ESURFINGCLIENT_XTEA_COMMON_H
#define ESURFINGCLIENT_XTEA_COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define XTEA_NUM_ROUNDS 32
#define XTEA_DELTA 0x9E3779B9u

static inline uint32_t xtea_get_be32(const uint8_t* data, const size_t offset)
{
    return (uint32_t)data[offset] << 24 | (uint32_t)data[offset + 1] << 16 |
           (uint32_t)data[offset + 2] << 8 | (uint32_t)data[offset + 3];
}

static inline void xtea_set_be32(uint8_t* data, const size_t offset, const uint32_t value)
{
    data[offset] = (uint8_t)(value >> 24 & 0xFF);
    data[offset + 1] = (uint8_t)(value >> 16 & 0xFF);
    data[offset + 2] = (uint8_t)(value >> 8 & 0xFF);
    data[offset + 3] = (uint8_t)(value & 0xFF);
}

static inline uint32_t xtea_bswap32(const uint32_t x)
{
    return x >> 24 | (x >> 8 & 0x0000FF00u) | (x << 8 & 0x00FF0000u) | x << 24;
}

static inline void xtea_encrypt_block(uint32_t* v0, uint32_t* v1, const uint32_t* key,
    const bool swap_key_endian)
{
    uint32_t sum = 0;
    const uint32_t ks[4] = {
        swap_key_endian ? xtea_bswap32(key[0]) : key[0],
        swap_key_endian ? xtea_bswap32(key[1]) : key[1],
        swap_key_endian ? xtea_bswap32(key[2]) : key[2],
        swap_key_endian ? xtea_bswap32(key[3]) : key[3]
    };
    for (int i = 0; i < XTEA_NUM_ROUNDS; ++i)
    {
        const uint32_t kB = ks[sum & 3u];
        *v0 += (*v1 << 4 ^ *v1 >> 5) + (sum ^ *v1) + kB;
        sum += XTEA_DELTA;
        const uint32_t kA = ks[(sum >> 11) & 3u];
        *v1 += (*v0 << 4 ^ *v0 >> 5) + (*v0 ^ sum) + kA;
    }
}

static inline void xtea_decrypt_block(uint32_t* v0, uint32_t* v1, const uint32_t* key,
    const bool swap_key_endian)
{
    uint32_t sum = XTEA_DELTA * XTEA_NUM_ROUNDS;
    const uint32_t ks[4] = {
        swap_key_endian ? xtea_bswap32(key[0]) : key[0],
        swap_key_endian ? xtea_bswap32(key[1]) : key[1],
        swap_key_endian ? xtea_bswap32(key[2]) : key[2],
        swap_key_endian ? xtea_bswap32(key[3]) : key[3]
    };
    for (int i = 0; i < XTEA_NUM_ROUNDS; ++i)
    {
        const uint32_t kA = ks[sum >> 11 & 3u];
        *v1 -= (*v0 << 4 ^ *v0 >> 5) + (*v0 ^ sum) + kA;
        sum -= XTEA_DELTA;
        const uint32_t kB = ks[sum & 3u];
        *v0 -= (*v1 << 4 ^ *v1 >> 5) + (sum ^ *v1) + kB;
    }
}

#endif // ESURFINGCLIENT_XTEA_COMMON_H
