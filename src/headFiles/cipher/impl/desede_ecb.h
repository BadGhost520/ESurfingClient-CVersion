#ifndef DESEDE_ECB_H
#define DESEDE_ECB_H

#include "../cipher_interface.h"
#include <stdint.h>

/**
 * 3DES-ECB 加解密算法头文件
 * 对应Kotlin的DESedeECB类
 */

/**
 * 创建3DES-ECB加解密实例
 * @param key1 第一个3DES密钥 (24字节)
 * @param key2 第二个3DES密钥 (24字节)
 * @return 加解密接口实例，失败返回NULL
 */
cipher_interface_t* create_desede_ecb_cipher(const uint8_t* key1, const uint8_t* key2);

#endif // DESEDE_ECB_H