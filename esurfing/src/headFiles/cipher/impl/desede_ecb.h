#ifndef DESEDE_ECB_H
#define DESEDE_ECB_H

#include "../CipherInterface.h"
#include <stdint.h>

/**
 * 创建3DES-ECB加解密实例
 * @param key1 第一个3DES密钥 (24字节)
 * @param key2 第二个3DES密钥 (24字节)
 * @return 加解密接口实例，失败返回NULL
 */
cipherInterfaceT* create_desede_ecb_cipher(const uint8_t* key1, const uint8_t* key2);

#endif // DESEDE_ECB_H