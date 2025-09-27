#ifndef AES_CBC_H
#define AES_CBC_H

#include "../cipher_interface.h"
#include <stdint.h>

/**
 * AES-CBC 加解密算法头文件
 * 对应Kotlin的AESCBC类
 */

/**
 * 创建AES-CBC加解密实例
 * @param key1 第一个AES密钥 (16字节)
 * @param key2 第二个AES密钥 (16字节)
 * @param iv1 第一个初始化向量 (16字节)
 * @param iv2 第二个初始化向量 (16字节)
 * @return 加解密接口实例，失败返回NULL
 */
cipher_interface_t* create_aes_cbc_cipher(const uint8_t* key1, const uint8_t* key2,
                                          const uint8_t* iv1, const uint8_t* iv2);

#endif // AES_CBC_H