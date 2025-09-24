//
// Created by bad_g on 2025/9/25.
//

#ifndef ESURFINGCLIENT_INTERFACE_H
#define ESURFINGCLIENT_INTERFACE_H

#include <stdint.h>

// 加密算法类型枚举
typedef enum {
    CIPHER_AES_CBC,
    CIPHER_AES_ECB,
    CIPHER_DESEDE_CBC,
    CIPHER_DESEDE_ECB,
    CIPHER_SM4_CBC,
    CIPHER_SM4_ECB,
    CIPHER_ZUC,
    CIPHER_MODXTEA,
    CIPHER_MODXTEA_IV
} cipher_type_t;

// 加密算法ID到类型的映射
typedef struct {
    const char* algo_id;
    cipher_type_t type;
} algo_mapping_t;

// 通用加密接口结构体
typedef struct cipher_interface {
    cipher_type_t type;
    const char* algo_id;  // 算法ID
    void* context;  // 指向具体算法的上下文

    // 函数指针
    int (*encrypt)(struct cipher_interface* self, const char* plaintext, char** ciphertext_hex);
    int (*decrypt)(struct cipher_interface* self, const char* ciphertext_hex, char** plaintext);
    void (*cleanup)(struct cipher_interface* self);
} cipher_interface_t;

// 密钥数据结构
typedef struct {
    // 通用密钥字段（用于AES、DES、SM4、ZUC等）
    const uint8_t* key1;
    size_t key1_len;
    const uint8_t* key2;
    size_t key2_len;

    // 32位密钥字段（用于ModXTEA）
    const uint32_t* key1_32;
    size_t key1_32_len;
    const uint32_t* key2_32;
    size_t key2_32_len;
    const uint32_t* key3_32;
    size_t key3_32_len;
} key_data_t;

// IV数据结构
typedef struct {
    // 通用IV字段（用于AES、DES、SM4、ZUC等）
    const uint8_t* iv;
    size_t iv_len;

    // 32位IV字段（用于ModXTEA）
    const uint32_t* iv_32;
    size_t iv_32_len;
} iv_data_t;

// 工厂函数声明
cipher_interface_t* cipher_factory_create(const char* algo_id);
void cipher_factory_destroy(cipher_interface_t* cipher);

// 算法ID映射函数
cipher_type_t get_cipher_type_by_algo_id(const char* algo_id);

// 密钥数据获取函数
int get_key_data(const char* algo_id, key_data_t* key_data);
int get_iv_data(const char* algo_id, iv_data_t* iv_data);

// 工具函数
char* bytes_to_hex(const uint8_t* bytes, size_t length);
uint8_t* hex_to_bytes(const char* hex, size_t* out_length);
void free_hex_string(char* hex_str);
void free_bytes(uint8_t* bytes);

// 错误码定义
#define CRYPTO_SUCCESS          0
#define CRYPTO_ERROR_INVALID_PARAM  -1
#define CRYPTO_ERROR_MEMORY     -2
#define CRYPTO_ERROR_UNSUPPORTED    -3
#define CRYPTO_ERROR_ENCRYPT    -4
#define CRYPTO_ERROR_DECRYPT    -5
#define CRYPTO_ERROR_UNKNOWN_ALGORITHM -6
#define CRYPTO_ERROR_ENCRYPTION_FAILED -7
#define CRYPTO_ERROR_MEMORY_ALLOCATION -8
#define CRYPTO_ERROR_DECRYPTION_FAILED -9

#endif //ESURFINGCLIENT_INTERFACE_H