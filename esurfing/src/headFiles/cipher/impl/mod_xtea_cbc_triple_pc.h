#ifndef MOD_XTEA_CBC_TRIPLE_PC_H
#define MOD_XTEA_CBC_TRIPLE_PC_H

#include "../CipherInterface.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

cipherInterfaceT* create_ab6c8_cipher(const uint32_t* key0, const uint32_t* key1,
                                        const uint32_t* key2, const uint32_t* iv);

#ifdef __cplusplus
}
#endif

#endif // MOD_XTEA_CBC_TRIPLE_PC_H