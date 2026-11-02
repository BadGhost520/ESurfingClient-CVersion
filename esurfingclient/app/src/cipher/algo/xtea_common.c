#include "xtea_common.h"

#include "cipher/CipherUtils.h"

void xtea_encrypt_block(uint32_t* v0, uint32_t* v1, const uint32_t* key, const bool swap_key_endian)
{
    uint32_t sum = 0;
    const uint32_t ks[4] = {
        swap_key_endian ? uint32_bswap(key[0]) : key[0],
        swap_key_endian ? uint32_bswap(key[1]) : key[1],
        swap_key_endian ? uint32_bswap(key[2]) : key[2],
        swap_key_endian ? uint32_bswap(key[3]) : key[3]
    };

    for (int i = 0; i < XTEA_NUM_ROUNDS; ++i)
    {
        const uint32_t k_b = ks[sum & 3u];
        *v0 += (*v1 << 4 ^ *v1 >> 5) + (sum ^ *v1) + k_b;
        sum += XTEA_DELTA;
        const uint32_t k_a = ks[(sum >> 11) & 3u];
        *v1 += (*v0 << 4 ^ *v0 >> 5) + (*v0 ^ sum) + k_a;
    }
}

void xtea_decrypt_block(uint32_t* v0, uint32_t* v1, const uint32_t* key, const bool swap_key_endian)
{
    uint32_t sum = XTEA_DELTA * XTEA_NUM_ROUNDS;
    const uint32_t ks[4] = {
        swap_key_endian ? uint32_bswap(key[0]) : key[0],
        swap_key_endian ? uint32_bswap(key[1]) : key[1],
        swap_key_endian ? uint32_bswap(key[2]) : key[2],
        swap_key_endian ? uint32_bswap(key[3]) : key[3]
    };

    for (int i = 0; i < XTEA_NUM_ROUNDS; ++i)
    {
        const uint32_t k_a = ks[sum >> 11 & 3u];
        *v1 -= (*v0 << 4 ^ *v0 >> 5) + (*v0 ^ sum) + k_a;
        sum -= XTEA_DELTA;
        const uint32_t k_b = ks[sum & 3u];
        *v0 -= (*v1 << 4 ^ *v1 >> 5) + (sum ^ *v1) + k_b;
    }
}
