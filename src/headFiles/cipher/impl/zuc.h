#ifndef ZUC_H
#define ZUC_H

#include "../cipher_interface.h"
#include <stdint.h>

/**
 * ZUC-128 加解密算法头文件
 * 对应Kotlin的ZUC类
 */

/**
 * 创建ZUC-128加解密实例
 * @param key ZUC密钥 (16字节)
 * @param iv 初始化向量 (16字节)
 * @return 加解密接口实例，失败返回NULL
 */
cipher_interface_t* create_zuc_cipher(const uint8_t* key, const uint8_t* iv);

#endif // ZUC_H