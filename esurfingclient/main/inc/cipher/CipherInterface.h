#ifndef ESURFINGCLIENT_CIPHERINTERFACE_H
#define ESURFINGCLIENT_CIPHERINTERFACE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct cipherInterface
{
    char* (*encrypt)(struct cipherInterface* self, const char* text);
    char* (*decrypt)(struct cipherInterface* self, const char* hex);
    void (*destroy)(struct cipherInterface* self);
    void* private_data;
} cipher_interface_t;

// extern cipher_interface_t* create_aes_cbc_android_old_cipher(
//     const uint8_t* key1,
//     const uint8_t* key2,
//     const uint8_t* iv);
//
// extern cipher_interface_t* create_mod_xtea_iv_android_old_cipher(
//     const uint32_t* key1,
//     const uint32_t* key2,
//     const uint32_t* key3,
//     const uint32_t* iv);
//
// extern cipher_interface_t* create_desede_cbc_android_old_cipher(
//     const uint8_t* key1,
//     const uint8_t* key2,
//     const uint8_t* iv1,
//     const uint8_t* iv2);
//
// extern cipher_interface_t* create_desede_ecb_android_old_cipher(
//     const uint8_t* key1,
//     const uint8_t* key2);
//
// extern cipher_interface_t* create_mod_xtea_android_old_cipher(
//     const uint32_t* key1,
//     const uint32_t* key2,
//     const uint32_t* key3);
//
// extern cipher_interface_t* create_aes_ecb_android_old_cipher(
//     const uint8_t* key1,
//     const uint8_t* key2);
//
// extern cipher_interface_t* create_sm4_cbc_android_old_cipher(
//     const uint8_t* key,
//     const uint8_t* iv);
//
// extern cipher_interface_t* create_sm4_ecb_android_old_cipher(
//     const uint8_t* key);
//
// extern cipher_interface_t* create_zuc_android_old_cipher(
//     const uint8_t* key,
//     const uint8_t* iv);

extern cipher_interface_t* create_ab6c8_linux_cipher(
    const uint32_t* key0,
    const uint32_t* key1,
    const uint32_t* key2,
    const uint32_t* iv);

extern cipher_interface_t* create_des_ecb_six_linux_cipher(
    const uint8_t* key0,
    const uint8_t* key1,
    const uint8_t* key2,
    const uint8_t* key3,
    const uint8_t* key4,
    const uint8_t* key5);

extern cipher_interface_t* create_desede_cbc_linux_cipher(
    const uint8_t* key1,
    const uint8_t* key2,
    const uint8_t* iv1,
    const uint8_t* iv2);

extern cipher_interface_t* create_mod_xtea_linux_cipher(
    const uint32_t* key1,
    const uint32_t* key2,
    const uint32_t* key3);

extern cipher_interface_t* create_aes_cbc_linux_cipher(
    const uint8_t* key1,
    const uint8_t* key2,
    const uint8_t* iv);

extern cipher_interface_t* create_aes_ecb_linux_cipher(
    const uint8_t* key1,
    const uint8_t* key2);

extern cipher_interface_t* create_snow3g_variant_android_cipher(
    const uint8_t* key,
    const uint8_t* iv);

extern cipher_interface_t* create_tea_triple_ecb_android_cipher(
    const uint8_t* key);

extern cipher_interface_t* create_tea_triple_cbc_android_cipher(
    const uint8_t* key,
    const uint8_t* iv);

extern cipher_interface_t* create_sm4_variant_cbc_android_cipher(
    const uint8_t* key,
    const uint8_t* iv);

extern cipher_interface_t* create_sm4_variant_ecb_android_cipher(
    const uint8_t* key);

extern cipher_interface_t* create_aes_double_cbc_android_cipher(
    const uint8_t* key,
    const uint8_t* iv);

extern cipher_interface_t* create_aes_double_ecb_android_cipher(
    const uint8_t* key);

extern cipher_interface_t* create_desede_double_cbc_android_cipher(
    const uint8_t* key,
    const uint8_t* iv);

extern cipher_interface_t* create_des_ecb_six_android_cipher(
    const uint8_t* key);

extern cipher_interface_t* create_xtea_triple_cbc_windows_cipher(
    const uint8_t* key,
     const uint8_t* iv);

/**
 * 销毁加解密工厂
 */
void destroy_cipher_factory();

/**
 * 初始化加解密工厂
 * @param algo_id 算法 ID
 * @return 初始化状态
 */
bool init_cipher(const char* algo_id);

/**
 * 加密函数
 * @param text 需要加密的文本
 * @return 加密后文本
 */
char* session_encrypt(const char* text);

/**
 * 解密函数
 * @param text 需要解密的文本
 * @return 加密后文本
 */
char* session_decrypt(const char* text);

#endif // ESURFINGCLIENT_CIPHERINTERFACE_H
