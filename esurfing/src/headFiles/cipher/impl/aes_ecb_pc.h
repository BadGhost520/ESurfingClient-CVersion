#ifndef AES_ECB_PC_H
#define AES_ECB_PC_H

#include <stdint.h>
#include "../CipherInterface.h"

#ifdef __cplusplus
extern "C" {
#endif

// 创建两层AES-ECB的PC版本（零填充，加密顺序：key2->key1，解密顺序：key1->key2）
cipher_interface_t* create_aes_ecb_pc_cipher(const uint8_t* key1, const uint8_t* key2);

#ifdef __cplusplus
}
#endif

#endif // AES_ECB_PC_H