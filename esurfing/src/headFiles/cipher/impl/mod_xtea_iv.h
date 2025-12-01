#ifndef MOD_XTEA_IV_H
#define MOD_XTEA_IV_H

#include "../CipherInterface.h"
#include <stdint.h>

/**
 * 创建ModXTEAIV加解密实例
 * @param key1 第一个XTEA密钥 (4个32位整数)
 * @param key2 第二个XTEA密钥 (4个32位整数)
 * @param key3 第三个XTEA密钥 (4个32位整数)
 * @param iv 初始化向量 (2个32位整数)
 * @return 加解密接口实例，失败返回NULL
 */
cipherInterfaceT* create_mod_xtea_iv_cipher(const uint32_t* key1, const uint32_t* key2, 
                                              const uint32_t* key3, const uint32_t* iv);

#endif // MOD_XTEA_IV_H