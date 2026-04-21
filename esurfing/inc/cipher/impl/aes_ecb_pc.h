#ifndef AES_ECB_PC_H
#define AES_ECB_PC_H

#include "cipher/CipherInterface.h"

#include <stdint.h>

/**
 * 创建AES-ECB-PC加解密实例
 * @param key1 第一个AES密钥 (16字节)
 * @param key2 第二个AES密钥 (16字节)
 * @return 加解密接口实例，失败返回NULL
 */
cipherInterfaceT* create_aes_ecb_pc_cipher(const uint8_t* key1, const uint8_t* key2);

#endif // AES_ECB_PC_H