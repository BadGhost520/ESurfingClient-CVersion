#ifndef SM4_ECB_H
#define SM4_ECB_H

#include "../CipherInterface.h"
#include <stdint.h>

/**
 * 创建SM4-ECB加解密实例
 * @param key SM4密钥 (16字节)
 * @return 加解密接口实例，失败返回NULL
 */
cipherInterfaceT* create_sm4_ecb_cipher(const uint8_t* key);

#endif // SM4_ECB_H