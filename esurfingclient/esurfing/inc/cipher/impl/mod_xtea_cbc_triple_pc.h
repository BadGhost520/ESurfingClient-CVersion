#ifndef MOD_XTEA_CBC_TRIPLE_PC_H
#define MOD_XTEA_CBC_TRIPLE_PC_H

#include "cipher/CipherInterface.h"

#include <stdint.h>

cipher_interface_t* create_ab6c8_cipher(const uint32_t* key0, const uint32_t* key1,
                                        const uint32_t* key2, const uint32_t* iv);

#endif // MOD_XTEA_CBC_TRIPLE_PC_H