#ifndef DES_ECB_SIX_PC_H
#define DES_ECB_SIX_PC_H

#include <stdint.h>
#include "../cipher_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

// DES-ECB 六阶段（PC）算法（官方命名）：
// 加密顺序：E(K3) -> D(K4) -> E(K5) -> E(K0) -> D(K1) -> E(K2)
// 解密顺序：D(K2) -> E(K1) -> D(K0) -> D(K5) -> E(K4) -> D(K3)
// 零填充到 8 字节倍数；密文输出为十六进制（大写）
cipher_interface_t* create_des_ecb_six_pc_cipher(
    const uint8_t* key0,
    const uint8_t* key1,
    const uint8_t* key2,
    const uint8_t* key3,
    const uint8_t* key4,
    const uint8_t* key5
);

#ifdef __cplusplus
}
#endif

#endif // DES_ECB_SIX_PC_H