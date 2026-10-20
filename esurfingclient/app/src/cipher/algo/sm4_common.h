#ifndef ESURFINGCLIENT_SM4_COMMON_H
#define ESURFINGCLIENT_SM4_COMMON_H

#include <stdint.h>

#define SM4_BLOCK_SIZE 16
#define SM4_KEY_SIZE 16

void sm4_key_expansion(const uint8_t* key, uint32_t* rk);
void sm4_encrypt_block(const uint8_t* in, uint8_t* out, const uint32_t* rk);
void sm4_decrypt_block(const uint8_t* in, uint8_t* out, const uint32_t* rk);

#endif // ESURFINGCLIENT_SM4_COMMON_H
