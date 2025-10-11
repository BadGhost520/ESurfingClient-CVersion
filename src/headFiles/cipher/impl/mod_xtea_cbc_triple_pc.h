#ifndef MOD_XTEA_CBC_TRIPLE_PC_H
#define MOD_XTEA_CBC_TRIPLE_PC_H

#include "../cipher_interface.h"
#include <stdint.h>

/**
 * AB6C8 三层 TEA 变体（CBC），使用固定 IV：
 * 解密顺序：K0 -> K1 -> K2，然后与上一个密文块异或；
 * 加密顺序：先与链值异或，再 K2 -> K1 -> K0。
 */

#ifdef __cplusplus
extern "C" {
#endif

cipher_interface_t* create_ab6c8_cipher(const uint32_t* key0, const uint32_t* key1,
                                        const uint32_t* key2, const uint32_t* iv);

#ifdef __cplusplus
}
#endif

#endif // MOD_XTEA_CBC_TRIPLE_PC_H