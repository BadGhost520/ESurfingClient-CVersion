#ifndef DESEDE_CBC_H
#define DESEDE_CBC_H

#include "../cipher_interface.h"
#include <stdint.h>

/**
 * 3DES-CBC 加解密算法头文件
 * 对应Kotlin的DESedeCBC类
 */

/**
 * 创建3DES-CBC加解密实例
 * @param key1 第一个3DES密钥 (24字节)
 * @param key2 第二个3DES密钥 (24字节)
 * @param iv1 第一个初始化向量 (8字节)
 * @param iv2 第二个初始化向量 (8字节)
 * @return 加解密接口实例，失败返回NULL
 */
cipher_interface_t* create_desede_cbc_cipher(const uint8_t* key1, const uint8_t* key2,
                                             const uint8_t* iv1, const uint8_t* iv2);

#endif // DESEDE_CBC_H