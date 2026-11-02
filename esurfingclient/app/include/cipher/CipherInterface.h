#ifndef ESURFINGCLIENT_CIPHERINTERFACE_H
#define ESURFINGCLIENT_CIPHERINTERFACE_H

#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define ALGO_ID_LEN 37

typedef struct cipherInterface
{
    char* (*encrypt)(struct cipherInterface* self, const char* text);
    char* (*decrypt)(struct cipherInterface* self, const char* hex);
    void (*destroy)(struct cipherInterface* self);
    void* private_data;
} cipher_interface_t;

extern cipher_interface_t* create_desede_ecb_cipher(
    const uint8_t* key1,
    const uint8_t* key2);

extern cipher_interface_t* create_zuc_cipher(
    const uint8_t* key,
    const uint8_t* iv);

extern cipher_interface_t* create_sm4_cbc_cipher(
    const uint8_t* key,
    const uint8_t* iv);

extern cipher_interface_t* create_sm4_ecb_cipher(
    const uint8_t* key);

extern cipher_interface_t* create_mod_xtea_iv_cipher(
    const uint32_t* key1,
    const uint32_t* key2,
    const uint32_t* key3,
    const uint32_t* iv);

extern cipher_interface_t* create_mod_xtea_cbc_triple_cipher(
    const uint32_t* key0,
    const uint32_t* key1,
    const uint32_t* key2,
    const uint32_t* iv);

extern cipher_interface_t* create_des_ecb_six_keys_cipher(
    const uint8_t* key0,
    const uint8_t* key1,
    const uint8_t* key2,
    const uint8_t* key3,
    const uint8_t* key4,
    const uint8_t* key5);

extern cipher_interface_t* create_desede_cbc_cipher(
    const uint8_t* key1,
    const uint8_t* key2,
    const uint8_t* iv1,
    const uint8_t* iv2);

// swap_key_endian: key 字是否先做 32 位字节序转换 (Linux 为 true, 旧 Android 为 false)
extern cipher_interface_t* create_mod_xtea_cipher(
    const uint32_t* key1,
    const uint32_t* key2,
    const uint32_t* key3,
    bool swap_key_endian);

// 双层 AES-128-CBC, 两层共用同一个 IV; iv == NULL 时使用全 0 IV
extern cipher_interface_t* create_aes_cbc_cipher(
    const uint8_t* key1,
    const uint8_t* key2,
    const uint8_t* iv);

extern cipher_interface_t* create_aes_ecb_cipher(
    const uint8_t* key1,
    const uint8_t* key2);

extern cipher_interface_t* create_snow3g_variant_cipher(
    const uint8_t* key,
    const uint8_t* iv);

extern cipher_interface_t* create_tea_triple_ecb_cipher(
    const uint8_t* key);

extern cipher_interface_t* create_tea_triple_cbc_cipher(
    const uint8_t* key,
    const uint8_t* iv);

extern cipher_interface_t* create_sm4_variant_cbc_cipher(
    const uint8_t* key,
    const uint8_t* iv);

extern cipher_interface_t* create_sm4_variant_ecb_cipher(
    const uint8_t* key);

extern cipher_interface_t* create_aes_double_cbc_cipher(
    const uint8_t* key,
    const uint8_t* iv);

extern cipher_interface_t* create_aes_double_ecb_cipher(
    const uint8_t* key);

extern cipher_interface_t* create_desede_double_cbc_cipher(
    const uint8_t* key,
    const uint8_t* iv);

extern cipher_interface_t* create_des_ecb_six_cipher(
    const uint8_t* key);

/**
 * @brief 销毁加解密工厂
 */
void destroy_cipher_factory(void);

/**
 * @brief 初始化加解密工厂
 * @param algo_id 算法 ID
 * @return 初始化状态
 */
bool init_cipher(const char* algo_id);

/**
 * @brief 判断 Algo-ID 是否有硬编码实现
 * @param algo_id 算法 ID
 * @return 是否存在
 */
bool cipher_supported(const char* algo_id);

/**
 * @brief 加密函数
 * @param text 需要加密的文本
 * @return 加密后文本
 */
char* session_encrypt(const char* text);

/**
 * @brief 解密函数
 * @param text 需要解密的文本
 * @return 解密后文本
 */
char* session_decrypt(const char* text);

#endif // ESURFINGCLIENT_CIPHERINTERFACE_H
