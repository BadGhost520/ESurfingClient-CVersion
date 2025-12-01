#ifndef ZUC_H
#define ZUC_H

#include "../CipherInterface.h"
#include <stdint.h>

/**
 * 创建ZUC-128加解密实例
 * @param key ZUC密钥 (16字节)
 * @param iv 初始化向量 (16字节)
 * @return 加解密接口实例，失败返回NULL
 */
cipherInterfaceT* create_zuc_cipher(const uint8_t* key, const uint8_t* iv);

#endif // ZUC_H