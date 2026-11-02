#ifndef ESURFINGCLIENT_XTEA_COMMON_H
#define ESURFINGCLIENT_XTEA_COMMON_H

#include <stdint.h>
#include <stdbool.h>

#define XTEA_NUM_ROUNDS 32
#define XTEA_DELTA 0x9E3779B9u

/**
 * 标准 32 轮 XTEA 单块加解密
 * @param v0 / v1 两块明文/密文 (按大端解释)
 * @param key 4 个 32 位密钥字
 * @param swap_key_endian 为真时先翻转密钥字的字节序
 */
void xtea_encrypt_block(uint32_t* v0, uint32_t* v1, const uint32_t* key, bool swap_key_endian);

void xtea_decrypt_block(uint32_t* v0, uint32_t* v1, const uint32_t* key, bool swap_key_endian);

#endif // ESURFINGCLIENT_XTEA_COMMON_H
