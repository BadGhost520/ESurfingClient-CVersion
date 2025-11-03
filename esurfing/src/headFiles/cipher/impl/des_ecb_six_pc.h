#ifndef DES_ECB_SIX_PC_H
#define DES_ECB_SIX_PC_H

#include <stdint.h>
#include "../CipherInterface.h"

#ifdef __cplusplus
extern "C" {
#endif

cipher_interface_t* create_des_ecb_six_pc_cipher(
    const uint8_t* key0,
    const uint8_t* key1,
    const uint8_t* key2,
    const uint8_t* key3,
    const uint8_t* key4,
    const uint8_t* key5
);

#ifdef __cplusplus
}
#endif

#endif // DES_ECB_SIX_PC_H