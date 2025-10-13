#ifndef MOD_XTEA_PC_H
#define MOD_XTEA_PC_H

#include "../cipher_interface.h"
#include <stdint.h>

/**
 * ModXTEA-PC 加解密算法头文件
 * 使用在PC环境中验证过的XTEA半轮顺序
 */

/**
 * 创建ModXTEA-PC加解密实例
 * @param key1 第一个XTEA密钥 (4个32位整数)
 * @param key2 第二个XTEA密钥 (4个32位整数)
 * @param key3 第三个XTEA密钥 (4个32位整数)
 * @return 加解密接口实例，失败返回NULL
 */
cipher_interface_t* create_mod_xtea_pc_cipher(const uint32_t* key1, const uint32_t* key2,
                                              const uint32_t* key3);

#endif // MOD_XTEA_PC_H