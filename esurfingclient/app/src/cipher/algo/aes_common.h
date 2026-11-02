#ifndef ESURFINGCLIENT_AES_COMMON_H
#define ESURFINGCLIENT_AES_COMMON_H

#include <stdint.h>

void aes128_key_expansion(const uint8_t* key, uint8_t* round_keys);
void aes128_encrypt_block(const uint8_t* in, uint8_t* out, const uint8_t* round_keys);
void aes128_decrypt_block(const uint8_t* in, uint8_t* out, const uint8_t* round_keys);

#endif // ESURFINGCLIENT_AES_COMMON_H
