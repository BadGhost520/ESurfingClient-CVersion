#ifndef SM4_CBC_H
#define SM4_CBC_H

#include "../cipher_interface.h"
#include <stdint.h>

/**
 * SM4-CBC 加解密算法头文件
 * 对应Kotlin的SM4CBC类
 */

/**
 * 创建SM4-CBC加解密实例
 * @param key SM4密钥 (16字节)
 * @param iv 初始化向量 (16字节)
 * @return 加解密接口实例，失败返回NULL
 */
cipher_interface_t* create_sm4_cbc_cipher(const uint8_t* key, const uint8_t* iv);

#endif // SM4_CBC_H