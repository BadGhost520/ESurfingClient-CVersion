#ifndef DESEDE_CBC_PC_H
#define DESEDE_CBC_PC_H

#include "../CipherInterface.h"
#include <stdint.h>

/**
 * 创建3DES-CBC(两层)加解密实例
 * @param key1 第一层的3DES密钥(24字节，包含3个8字节子钥)
 * @param key2 第二层的3DES密钥(24字节，包含3个8字节子钥)
 * @param iv1 第一层 CBC IV (8字节)
 * @param iv2 第二层 CBC IV (8字节)
 */
cipher_interface_t* create_desede_cbc_pc_cipher(const uint8_t* key1, const uint8_t* key2,
                                                const uint8_t* iv1, const uint8_t* iv2);

#endif // DESEDE_CBC_PC_H