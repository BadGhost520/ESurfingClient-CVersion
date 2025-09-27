#ifndef SM4_ECB_H
#define SM4_ECB_H

#include "../cipher_interface.h"
#include <stdint.h>

/**
 * SM4-ECB 加解密算法头文件
 * 对应Kotlin的SM4ECB类
 */

/**
 * 创建SM4-ECB加解密实例
 * @param key SM4密钥 (16字节)
 * @return 加解密接口实例，失败返回NULL
 */
cipher_interface_t* create_sm4_ecb_cipher(const uint8_t* key);

#endif // SM4_ECB_H