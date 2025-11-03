#ifndef SM4_CBC_H
#define SM4_CBC_H

#include "../CipherInterface.h"
#include <stdint.h>

/**
 * 创建SM4-CBC加解密实例
 * @param key SM4密钥 (16字节)
 * @param iv 初始化向量 (16字节)
 * @return 加解密接口实例，失败返回NULL
 */
cipher_interface_t* createSm4CbcCipher(const uint8_t* key, const uint8_t* iv);

#endif // SM4_CBC_H