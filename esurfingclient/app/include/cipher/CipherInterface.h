#ifndef ESURFINGCLIENT_CIPHERINTERFACE_H
#define ESURFINGCLIENT_CIPHERINTERFACE_H

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#define ALGO_ID_LEN 37

typedef struct cipherInterface
{
    char* (*encrypt)(struct cipherInterface* self, const char* text);
    char* (*decrypt)(struct cipherInterface* self, const char* hex);
    void (*destroy)(struct cipherInterface* self);
    void* private_data;
} cipher_interface_t;

typedef struct
{
    char algo_id[ALGO_ID_LEN];
    uint8_t* key;
    size_t key_len;
    uint8_t* iv;
    size_t iv_len;
    char* js;
} ios_zsm_blob_t;

// cipher_interface_t* create_aes_cbc_android_old_cipher(
//     const uint8_t* key1,
//     const uint8_t* key2,
//     const uint8_t* iv);
//
// cipher_interface_t* create_mod_xtea_iv_android_old_cipher(
//     const uint32_t* key1,
//     const uint32_t* key2,
//     const uint32_t* key3,
//     const uint32_t* iv);
//
// cipher_interface_t* create_desede_cbc_android_old_cipher(
//     const uint8_t* key1,
//     const uint8_t* key2,
//     const uint8_t* iv1,
//     const uint8_t* iv2);
//
// cipher_interface_t* create_desede_ecb_android_old_cipher(
//     const uint8_t* key1,
//     const uint8_t* key2);
//
// cipher_interface_t* create_mod_xtea_android_old_cipher(
//     const uint32_t* key1,
//     const uint32_t* key2,
//     const uint32_t* key3);
//
// cipher_interface_t* create_aes_ecb_android_old_cipher(
//     const uint8_t* key1,
//     const uint8_t* key2);
//
// cipher_interface_t* create_sm4_cbc_android_old_cipher(
//     const uint8_t* key,
//     const uint8_t* iv);
//
// cipher_interface_t* create_sm4_ecb_android_old_cipher(
//     const uint8_t* key);
//
// cipher_interface_t* create_zuc_android_old_cipher(
//     const uint8_t* key,
//     const uint8_t* iv);

cipher_interface_t* create_ab6c8_linux_cipher(
    const uint32_t* key0,
    const uint32_t* key1,
    const uint32_t* key2,
    const uint32_t* iv);

cipher_interface_t* create_des_ecb_six_linux_cipher(
    const uint8_t* key0,
    const uint8_t* key1,
    const uint8_t* key2,
    const uint8_t* key3,
    const uint8_t* key4,
    const uint8_t* key5);

cipher_interface_t* create_desede_cbc_linux_cipher(
    const uint8_t* key1,
    const uint8_t* key2,
    const uint8_t* iv1,
    const uint8_t* iv2);

cipher_interface_t* create_mod_xtea_linux_cipher(
    const uint32_t* key1,
    const uint32_t* key2,
    const uint32_t* key3);

cipher_interface_t* create_aes_cbc_linux_cipher(
    const uint8_t* key1,
    const uint8_t* key2);

cipher_interface_t* create_aes_ecb_linux_cipher(
    const uint8_t* key1,
    const uint8_t* key2);

cipher_interface_t* create_snow3g_variant_android_cipher(
    const uint8_t* key,
    const uint8_t* iv);

cipher_interface_t* create_tea_triple_ecb_android_cipher(
    const uint8_t* key);

cipher_interface_t* create_tea_triple_cbc_android_cipher(
    const uint8_t* key,
    const uint8_t* iv);

cipher_interface_t* create_sm4_variant_cbc_android_cipher(
    const uint8_t* key,
    const uint8_t* iv);

cipher_interface_t* create_sm4_variant_ecb_android_cipher(
    const uint8_t* key);

cipher_interface_t* create_aes_double_cbc_android_cipher(
    const uint8_t* key,
    const uint8_t* iv);

cipher_interface_t* create_aes_double_ecb_android_cipher(
    const uint8_t* key);

cipher_interface_t* create_desede_double_cbc_android_cipher(
    const uint8_t* key,
    const uint8_t* iv);

cipher_interface_t* create_des_ecb_six_android_cipher(
    const uint8_t* key);

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

/**
 * 判断 ticket 响应是否为 PacketTunnel IZsmModLoad 动态模块.
 * 头部是两个 Pascal 字符串, 随后 LZMA packed type nibble == 2.
 * 这种 ZSM 的 AID 不在 Android/Linux CipherFactory 里.
 */
bool looks_like_ios_zsm(const uint8_t* data, size_t length);

/**
 * 销毁加解密工厂
 */
void destroy_cipher_factory();

/**
 * 从 iOS PacketTunnel ZSM 解包密钥并初始化加解密工厂.
 * ZSM 不是 Android/Linux 那种 UUID→硬编码密钥表, 正文是 TEA + LZMA 后的 JS,
 * 密钥在 cdckey/cdciv, 算法由 JS 的 var codex = 0xNN 或 cdy(type, ...) 决定.
 *
 * @param data ZSM 原始字节
 * @param length ZSM 长度
 * @param algo_id_out 可选, 写入头部 str2 的 Algo-ID (大写 UUID)
 * @return 是否成功
 */
bool init_ios_cipher_from_zsm(const uint8_t* data, size_t length, char* algo_id_out);

bool init_ios_cipher_from_blob(int8_t type, ios_zsm_blob_t blob);

/**
 * 初始化加解密工厂
 * @param algo_id 算法 ID
 * @return 初始化状态
 */
bool init_cipher(const char* algo_id);

#endif
