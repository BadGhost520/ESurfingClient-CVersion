#ifndef AES_CBC_PC_H
#define AES_CBC_PC_H

#include "../cipher_interface.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// AES-CBC 双层（PC），解密按密文内置 IV：先 key2+outerIV，再 key1+innerIV
cipher_interface_t* create_aes_cbc_pc_cipher(const uint8_t* key1, const uint8_t* key2);

#ifdef __cplusplus
}
#endif

#endif // AES_CBC_PC_H