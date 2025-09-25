//
// Created by bad_g on 2025/9/25.
//
#include <stdlib.h>
#include <string.h>

#include "../headFiles/cipher/Interface.h"
#include "../headFiles/cipher/KeyData.h"
#include "../headFiles/cipher/Utils.h"
#include "../headFiles/cipher/impl/SM4.h"
#include "../headFiles/cipher/impl/Zuc128.h"
#include "../headFiles/cipher/impl/AES.h"
#include "../headFiles/cipher/impl/DES.h"
#include "../headFiles/cipher/impl/ModXTEA.h"

// 算法ID到类型的映射表
static const algo_mapping_t ALGO_MAPPINGS[] = {
    {"CAFBCBAD-B6E7-4CAB-8A67-14D39F00CE1E", CIPHER_AES_CBC},
    {"A474B1C2-3DE0-4EA2-8C5F-7093409CE6C4", CIPHER_AES_ECB},
    {"5BFBA864-BBA9-42DB-8EAD-49B5F412BD81", CIPHER_DESEDE_CBC},
    {"6E0B65FF-0B5B-459C-8FCE-EC7F2BEA9FF5", CIPHER_DESEDE_ECB},
    {"B809531F-0007-4B5B-923B-4BD560398113", CIPHER_ZUC},
    {"F3974434-C0DD-4C20-9E87-DDB6814A1C48", CIPHER_SM4_CBC},
    {"ED382482-F72C-4C41-A76D-28EEA0F1F2AF", CIPHER_SM4_ECB},
    {"B3047D4E-67DF-4864-A6A5-DF9B9E525C79", CIPHER_MODXTEA},
    {"C32C68F9-CA81-4260-A329-BBAFD1A9CCD1", CIPHER_MODXTEA_IV}
};

static const size_t ALGO_MAPPINGS_COUNT = sizeof(ALGO_MAPPINGS) / sizeof(algo_mapping_t);

// 根据算法ID获取加密类型
cipher_type_t get_cipher_type_by_algo_id(const char* algo_id) {
    if (!algo_id) return -1;

    for (size_t i = 0; i < ALGO_MAPPINGS_COUNT; i++) {
        if (strcmp(algo_id, ALGO_MAPPINGS[i].algo_id) == 0) {
            return ALGO_MAPPINGS[i].type;
        }
    }

    return -1; // 未找到对应的算法类型
}

// 获取密钥数据
int get_key_data(const char* algo_id, key_data_t* key_data) {
    if (!algo_id || !key_data) return CRYPTO_ERROR_INVALID_PARAM;

    memset(key_data, 0, sizeof(key_data_t));

    if (strcmp(algo_id, "CAFBCBAD-B6E7-4CAB-8A67-14D39F00CE1E") == 0) {
        // AES/CBC
        key_data->key1 = key1_CAFBCBAD_B6E7_4CAB_8A67_14D39F00CE1E;
        key_data->key1_len = 16;
        key_data->key2 = key2_CAFBCBAD_B6E7_4CAB_8A67_14D39F00CE1E;
        key_data->key2_len = 16;
    } else if (strcmp(algo_id, "A474B1C2-3DE0-4EA2-8C5F-7093409CE6C4") == 0) {
        // AES/ECB
        key_data->key1 = key1_A474B1C2_3DE0_4EA2_8C5F_7093409CE6C4;
        key_data->key1_len = 16;
        key_data->key2 = key2_A474B1C2_3DE0_4EA2_8C5F_7093409CE6C4;
        key_data->key2_len = 16;
    } else if (strcmp(algo_id, "5BFBA864-BBA9-42DB-8EAD-49B5F412BD81") == 0) {
        // DESede/CBC
        key_data->key1 = key1_5BFBA864_BBA9_42DB_8EAD_49B5F412BD81;
        key_data->key1_len = 24;
        key_data->key2 = key2_5BFBA864_BBA9_42DB_8EAD_49B5F412BD81;
        key_data->key2_len = 24;
    } else if (strcmp(algo_id, "6E0B65FF-0B5B-459C-8FCE-EC7F2BEA9FF5") == 0) {
        // DESede/ECB
        key_data->key1 = key1_6E0B65FF_0B5B_459C_8FCE_EC7F2BEA9FF5;
        key_data->key1_len = 24;
        key_data->key2 = key2_6E0B65FF_0B5B_459C_8FCE_EC7F2BEA9FF5;
        key_data->key2_len = 24;
    } else if (strcmp(algo_id, "B809531F-0007-4B5B-923B-4BD560398113") == 0) {
        // ZUC
        key_data->key1 = key_B809531F_0007_4B5B_923B_4BD560398113;
        key_data->key1_len = 16;
    } else if (strcmp(algo_id, "F3974434-C0DD-4C20-9E87-DDB6814A1C48") == 0) {
        // SM4/CBC
        key_data->key1 = key_F3974434_C0DD_4C20_9E87_DDB6814A1C48;
        key_data->key1_len = 16;
    } else if (strcmp(algo_id, "ED382482-F72C-4C41-A76D-28EEA0F1F2AF") == 0) {
        // SM4/ECB
        key_data->key1 = key_ED382482_F72C_4C41_A76D_28EEA0F1F2AF;
        key_data->key1_len = 16;
    } else if (strcmp(algo_id, "B3047D4E-67DF-4864-A6A5-DF9B9E525C79") == 0) {
        // ModXTEA
        key_data->key1_32 = key1_B3047D4E_67DF_4864_A6A5_DF9B9E525C79;
        key_data->key1_32_len = 4;
        key_data->key2_32 = key2_B3047D4E_67DF_4864_A6A5_DF9B9E525C79;
        key_data->key2_32_len = 4;
        key_data->key3_32 = key3_B3047D4E_67DF_4864_A6A5_DF9B9E525C79;
        key_data->key3_32_len = 4;
    } else if (strcmp(algo_id, "C32C68F9-CA81-4260-A329-BBAFD1A9CCD1") == 0) {
        // ModXTEA with IV
        key_data->key1_32 = key1_C32C68F9_CA81_4260_A329_BBAFD1A9CCD1;
        key_data->key1_32_len = 4;
        key_data->key2_32 = key2_C32C68F9_CA81_4260_A329_BBAFD1A9CCD1;
        key_data->key2_32_len = 4;
        key_data->key3_32 = key3_C32C68F9_CA81_4260_A329_BBAFD1A9CCD1;
        key_data->key3_32_len = 4;
    } else {
        return CRYPTO_ERROR_UNKNOWN_ALGORITHM;
    }

    return CRYPTO_SUCCESS;
}

// 获取IV数据
int get_iv_data(const char* algo_id, iv_data_t* iv_data) {
    if (!algo_id || !iv_data) return CRYPTO_ERROR_INVALID_PARAM;

    memset(iv_data, 0, sizeof(iv_data_t));

    if (strcmp(algo_id, "CAFBCBAD-B6E7-4CAB-8A67-14D39F00CE1E") == 0) {
        // AES/CBC
        iv_data->iv = iv_CAFBCBAD_B6E7_4CAB_8A67_14D39F00CE1E;
        iv_data->iv_len = 16;
    } else if (strcmp(algo_id, "5BFBA864-BBA9-42DB-8EAD-49B5F412BD81") == 0) {
        // DESede/CBC
        iv_data->iv = iv_5BFBA864_BBA9_42DB_8EAD_49B5F412BD81;
        iv_data->iv_len = 8;
    } else if (strcmp(algo_id, "B809531F-0007-4B5B-923B-4BD560398113") == 0) {
        // ZUC
        iv_data->iv = iv_B809531F_0007_4B5B_923B_4BD560398113;
        iv_data->iv_len = 16;
    } else if (strcmp(algo_id, "F3974434-C0DD-4C20-9E87-DDB6814A1C48") == 0) {
        // SM4/CBC
        iv_data->iv = iv_F3974434_C0DD_4C20_9E87_DDB6814A1C48;
        iv_data->iv_len = 16;
    } else if (strcmp(algo_id, "C32C68F9-CA81-4260-A329-BBAFD1A9CCD1") == 0) {
        // ModXTEA with IV
        iv_data->iv_32 = iv_C32C68F9_CA81_4260_A329_BBAFD1A9CCD1;
        iv_data->iv_32_len = 2;
    }

    return CRYPTO_SUCCESS;
}

// AES加密函数
static int aes_cipher_encrypt(cipher_interface_t* cipher, const char* plaintext, char** ciphertext) {
    if (!cipher || !plaintext || !ciphertext) return CRYPTO_ERROR_INVALID_PARAM;

    key_data_t* key_data = cipher->context;
    size_t plaintext_len = strlen(plaintext);
    uint8_t* encrypted_data;
    size_t encrypted_len;

    int result;
    if (cipher->type == CIPHER_AES_CBC) {
        iv_data_t iv_data;
        get_iv_data(cipher->algo_id, &iv_data);
        result = aes_double_encrypt_cbc(key_data->key1, key_data->key2, iv_data.iv,
                                       (const uint8_t*)plaintext, plaintext_len,
                                       &encrypted_data, &encrypted_len);
    } else {
        result = aes_double_encrypt_ecb(key_data->key1, key_data->key2,
                                       (const uint8_t*)plaintext, plaintext_len,
                                       &encrypted_data, &encrypted_len);
    }

    if (result != 0) return CRYPTO_ERROR_ENCRYPTION_FAILED;

    // 转换为十六进制字符串
    *ciphertext = bytes_to_hex_string(encrypted_data, encrypted_len);
    free(encrypted_data);

    return (*ciphertext != NULL) ? CRYPTO_SUCCESS : CRYPTO_ERROR_MEMORY_ALLOCATION;
}

// AES解密函数
static int aes_cipher_decrypt(cipher_interface_t* cipher, const char* ciphertext, char** plaintext) {
    if (!cipher || !ciphertext || !plaintext) return CRYPTO_ERROR_INVALID_PARAM;

    key_data_t* key_data = cipher->context;
    size_t ciphertext_len;
    uint8_t* encrypted_data = hex_string_to_bytes(ciphertext, &ciphertext_len);
    if (!encrypted_data) return CRYPTO_ERROR_INVALID_PARAM;

    uint8_t* decrypted_data;
    size_t decrypted_len;

    int result;
    if (cipher->type == CIPHER_AES_CBC) {
        iv_data_t iv_data;
        get_iv_data(cipher->algo_id, &iv_data);
        result = aes_double_decrypt_cbc(key_data->key1, key_data->key2, iv_data.iv,
                                       encrypted_data, ciphertext_len,
                                       &decrypted_data, &decrypted_len);
    } else {
        result = aes_double_decrypt_ecb(key_data->key1, key_data->key2,
                                       encrypted_data, ciphertext_len,
                                       &decrypted_data, &decrypted_len);
    }

    free(encrypted_data);
    if (result != 0) return CRYPTO_ERROR_DECRYPTION_FAILED;

    // 转换为字符串
    *plaintext = malloc(decrypted_len + 1);
    if (!*plaintext) {
        free(decrypted_data);
        return CRYPTO_ERROR_MEMORY_ALLOCATION;
    }

    memcpy(*plaintext, decrypted_data, decrypted_len);
    (*plaintext)[decrypted_len] = '\0';
    free(decrypted_data);

    return CRYPTO_SUCCESS;
}

// 3DES加密函数
static int tdes_cipher_encrypt(cipher_interface_t* cipher, const char* plaintext, char** ciphertext) {
    if (!cipher || !plaintext || !ciphertext) return CRYPTO_ERROR_INVALID_PARAM;

    key_data_t* key_data = cipher->context;
    size_t plaintext_len = strlen(plaintext);
    uint8_t* encrypted_data;
    size_t encrypted_len;

    int result;
    if (cipher->type == CIPHER_DESEDE_CBC) {
        iv_data_t iv_data;
        get_iv_data(cipher->algo_id, &iv_data);
        result = tdes_double_encrypt_cbc(key_data->key1, key_data->key2, iv_data.iv,
                                        (const uint8_t*)plaintext, plaintext_len,
                                        &encrypted_data, &encrypted_len);
    } else {
        result = tdes_double_encrypt_ecb(key_data->key1, key_data->key2,
                                        (const uint8_t*)plaintext, plaintext_len,
                                        &encrypted_data, &encrypted_len);
    }

    if (result != 0) return CRYPTO_ERROR_ENCRYPTION_FAILED;

    // 转换为十六进制字符串
    *ciphertext = bytes_to_hex_string(encrypted_data, encrypted_len);
    free(encrypted_data);

    return (*ciphertext != NULL) ? CRYPTO_SUCCESS : CRYPTO_ERROR_MEMORY_ALLOCATION;
}

// 3DES解密函数
static int tdes_cipher_decrypt(cipher_interface_t* cipher, const char* ciphertext, char** plaintext) {
    if (!cipher || !ciphertext || !plaintext) return CRYPTO_ERROR_INVALID_PARAM;

    key_data_t* key_data = cipher->context;
    size_t ciphertext_len;
    uint8_t* encrypted_data = hex_string_to_bytes(ciphertext, &ciphertext_len);
    if (!encrypted_data) return CRYPTO_ERROR_INVALID_PARAM;

    uint8_t* decrypted_data;
    size_t decrypted_len;

    int result;
    if (cipher->type == CIPHER_DESEDE_CBC) {
        iv_data_t iv_data;
        get_iv_data(cipher->algo_id, &iv_data);
        result = tdes_double_decrypt_cbc(key_data->key1, key_data->key2, iv_data.iv,
                                        encrypted_data, ciphertext_len,
                                        &decrypted_data, &decrypted_len);
    } else {
        result = tdes_double_decrypt_ecb(key_data->key1, key_data->key2,
                                        encrypted_data, ciphertext_len,
                                        &decrypted_data, &decrypted_len);
    }

    free(encrypted_data);
    if (result != 0) return CRYPTO_ERROR_DECRYPTION_FAILED;

    // 转换为字符串
    *plaintext = malloc(decrypted_len + 1);
    if (!*plaintext) {
        free(decrypted_data);
        return CRYPTO_ERROR_MEMORY_ALLOCATION;
    }

    memcpy(*plaintext, decrypted_data, decrypted_len);
    (*plaintext)[decrypted_len] = '\0';
    free(decrypted_data);

    return CRYPTO_SUCCESS;
}

// SM4加密函数
static int sm4_cipher_encrypt(cipher_interface_t* cipher, const char* plaintext, char** ciphertext) {
    if (!cipher || !plaintext || !ciphertext) return CRYPTO_ERROR_INVALID_PARAM;

    key_data_t* key_data = cipher->context;
    size_t plaintext_len = strlen(plaintext);
    uint8_t* encrypted_data;
    size_t encrypted_len;

    int result;
    if (cipher->type == CIPHER_SM4_CBC) {
        iv_data_t iv_data;
        get_iv_data(cipher->algo_id, &iv_data);
        result = sm4_encrypt_cbc(key_data->key1, iv_data.iv,
                                (const uint8_t*)plaintext, plaintext_len,
                                &encrypted_data, &encrypted_len);
    } else {
        result = sm4_encrypt_ecb(key_data->key1,
                                (const uint8_t*)plaintext, plaintext_len,
                                &encrypted_data, &encrypted_len);
    }

    if (result != 0) return CRYPTO_ERROR_ENCRYPTION_FAILED;

    // 转换为十六进制字符串
    *ciphertext = bytes_to_hex_string(encrypted_data, encrypted_len);
    free(encrypted_data);

    return (*ciphertext != NULL) ? CRYPTO_SUCCESS : CRYPTO_ERROR_MEMORY_ALLOCATION;
}

// SM4解密函数
static int sm4_cipher_decrypt(cipher_interface_t* cipher, const char* ciphertext, char** plaintext) {
    if (!cipher || !ciphertext || !plaintext) return CRYPTO_ERROR_INVALID_PARAM;

    key_data_t* key_data = cipher->context;
    size_t ciphertext_len;
    uint8_t* encrypted_data = hex_string_to_bytes(ciphertext, &ciphertext_len);
    if (!encrypted_data) return CRYPTO_ERROR_INVALID_PARAM;

    uint8_t* decrypted_data;
    size_t decrypted_len;

    int result;
    if (cipher->type == CIPHER_SM4_CBC) {
        iv_data_t iv_data;
        get_iv_data(cipher->algo_id, &iv_data);
        result = sm4_decrypt_cbc(key_data->key1, iv_data.iv,
                                encrypted_data, ciphertext_len,
                                &decrypted_data, &decrypted_len);
    } else {
        result = sm4_decrypt_ecb(key_data->key1,
                                encrypted_data, ciphertext_len,
                                &decrypted_data, &decrypted_len);
    }

    free(encrypted_data);
    if (result != 0) return CRYPTO_ERROR_DECRYPTION_FAILED;

    // 转换为字符串
    *plaintext = malloc(decrypted_len + 1);
    if (!*plaintext) {
        free(decrypted_data);
        return CRYPTO_ERROR_MEMORY_ALLOCATION;
    }

    memcpy(*plaintext, decrypted_data, decrypted_len);
    (*plaintext)[decrypted_len] = '\0';
    free(decrypted_data);

    return CRYPTO_SUCCESS;
}

// ZUC加密函数
static int zuc_cipher_encrypt(cipher_interface_t* cipher, const char* plaintext, char** ciphertext) {
    if (!cipher || !plaintext || !ciphertext) return CRYPTO_ERROR_INVALID_PARAM;

    key_data_t* key_data = cipher->context;
    iv_data_t iv_data;
    get_iv_data(cipher->algo_id, &iv_data);

    size_t plaintext_len = strlen(plaintext);
    uint8_t* encrypted_data;
    size_t encrypted_len;

    int result = zuc128_encrypt(key_data->key1, iv_data.iv,
                               (const uint8_t*)plaintext, plaintext_len,
                               &encrypted_data, &encrypted_len);

    if (result != 0) return CRYPTO_ERROR_ENCRYPTION_FAILED;

    // 转换为十六进制字符串
    *ciphertext = bytes_to_hex_string(encrypted_data, encrypted_len);
    free(encrypted_data);

    return (*ciphertext != NULL) ? CRYPTO_SUCCESS : CRYPTO_ERROR_MEMORY_ALLOCATION;
}

// ZUC解密函数
static int zuc_cipher_decrypt(cipher_interface_t* cipher, const char* ciphertext, char** plaintext) {
    if (!cipher || !ciphertext || !plaintext) return CRYPTO_ERROR_INVALID_PARAM;

    key_data_t* key_data = cipher->context;
    iv_data_t iv_data;
    get_iv_data(cipher->algo_id, &iv_data);

    size_t ciphertext_len;
    uint8_t* encrypted_data = hex_string_to_bytes(ciphertext, &ciphertext_len);
    if (!encrypted_data) return CRYPTO_ERROR_INVALID_PARAM;

    uint8_t* decrypted_data;
    size_t decrypted_len;

    int result = zuc128_decrypt(key_data->key1, iv_data.iv,
                               encrypted_data, ciphertext_len,
                               &decrypted_data, &decrypted_len);

    free(encrypted_data);
    if (result != 0) return CRYPTO_ERROR_DECRYPTION_FAILED;

    // 转换为字符串
    *plaintext = malloc(decrypted_len + 1);
    if (!*plaintext) {
        free(decrypted_data);
        return CRYPTO_ERROR_MEMORY_ALLOCATION;
    }

    memcpy(*plaintext, decrypted_data, decrypted_len);
    (*plaintext)[decrypted_len] = '\0';
    free(decrypted_data);

    return CRYPTO_SUCCESS;
}

// ModXTEA加密函数
static int modxtea_cipher_encrypt(cipher_interface_t* cipher, const char* plaintext, char** ciphertext) {
    if (!cipher || !plaintext || !ciphertext) return CRYPTO_ERROR_INVALID_PARAM;

    key_data_t* key_data = cipher->context;
    size_t plaintext_len = strlen(plaintext);
    uint8_t* encrypted_data;
    size_t encrypted_len;

    int result;
    if (cipher->type == CIPHER_MODXTEA_IV) {
        iv_data_t iv_data;
        get_iv_data(cipher->algo_id, &iv_data);
        result = modxtea_encrypt_iv(key_data->key1_32, key_data->key2_32, key_data->key3_32,
                                   iv_data.iv_32, (const uint8_t*)plaintext, plaintext_len,
                                   &encrypted_data, &encrypted_len);
    } else {
        result = modxtea_encrypt(key_data->key1_32, key_data->key2_32, key_data->key3_32,
                                (const uint8_t*)plaintext, plaintext_len,
                                &encrypted_data, &encrypted_len);
    }

    if (result != 0) return CRYPTO_ERROR_ENCRYPTION_FAILED;

    // 转换为十六进制字符串
    *ciphertext = bytes_to_hex_string(encrypted_data, encrypted_len);
    free(encrypted_data);

    return (*ciphertext != NULL) ? CRYPTO_SUCCESS : CRYPTO_ERROR_MEMORY_ALLOCATION;
}

// ModXTEA解密函数
static int modxtea_cipher_decrypt(cipher_interface_t* cipher, const char* ciphertext, char** plaintext) {
    if (!cipher || !ciphertext || !plaintext) return CRYPTO_ERROR_INVALID_PARAM;

    key_data_t* key_data = cipher->context;
    size_t ciphertext_len;
    uint8_t* encrypted_data = hex_string_to_bytes(ciphertext, &ciphertext_len);
    if (!encrypted_data) return CRYPTO_ERROR_INVALID_PARAM;

    uint8_t* decrypted_data;
    size_t decrypted_len;

    int result;
    if (cipher->type == CIPHER_MODXTEA_IV) {
        iv_data_t iv_data;
        get_iv_data(cipher->algo_id, &iv_data);
        result = modxtea_decrypt_iv(key_data->key1_32, key_data->key2_32, key_data->key3_32,
                                   iv_data.iv_32, encrypted_data, ciphertext_len,
                                   &decrypted_data, &decrypted_len);
    } else {
        result = modxtea_decrypt(key_data->key1_32, key_data->key2_32, key_data->key3_32,
                                encrypted_data, ciphertext_len,
                                &decrypted_data, &decrypted_len);
    }

    free(encrypted_data);
    if (result != 0) return CRYPTO_ERROR_DECRYPTION_FAILED;

    // 转换为字符串
    *plaintext = malloc(decrypted_len + 1);
    if (!*plaintext) {
        free(decrypted_data);
        return CRYPTO_ERROR_MEMORY_ALLOCATION;
    }

    memcpy(*plaintext, decrypted_data, decrypted_len);
    (*plaintext)[decrypted_len] = '\0';
    free(decrypted_data);

    return CRYPTO_SUCCESS;
}

// 清理函数
static void cipher_cleanup(cipher_interface_t* cipher) {
    if (cipher && cipher->context) {
        free(cipher->context);
        cipher->context = NULL;
    }
}

// 创建加密接口
cipher_interface_t* cipher_factory_create(const char* algo_id) {
    if (!algo_id) return NULL;

    cipher_type_t type = get_cipher_type_by_algo_id(algo_id);
    if (type == -1) return NULL;

    cipher_interface_t* cipher = malloc(sizeof(cipher_interface_t));
    if (!cipher) return NULL;

    cipher->type = type;
    cipher->algo_id = algo_id;
    cipher->cleanup = cipher_cleanup;

    // 分配并初始化上下文
    key_data_t* key_data = malloc(sizeof(key_data_t));
    if (!key_data) {
        free(cipher);
        return NULL;
    }

    if (get_key_data(algo_id, key_data) != CRYPTO_SUCCESS) {
        free(key_data);
        free(cipher);
        return NULL;
    }

    cipher->context = key_data;

    // 设置函数指针
    switch (type) {
        case CIPHER_AES_CBC:
        case CIPHER_AES_ECB:
            cipher->encrypt = aes_cipher_encrypt;
            cipher->decrypt = aes_cipher_decrypt;
            break;
        case CIPHER_DESEDE_CBC:
        case CIPHER_DESEDE_ECB:
            cipher->encrypt = tdes_cipher_encrypt;
            cipher->decrypt = tdes_cipher_decrypt;
            break;
        case CIPHER_SM4_CBC:
        case CIPHER_SM4_ECB:
            cipher->encrypt = sm4_cipher_encrypt;
            cipher->decrypt = sm4_cipher_decrypt;
            break;
        case CIPHER_ZUC:
            cipher->encrypt = zuc_cipher_encrypt;
            cipher->decrypt = zuc_cipher_decrypt;
            break;
        case CIPHER_MODXTEA:
        case CIPHER_MODXTEA_IV:
            cipher->encrypt = modxtea_cipher_encrypt;
            cipher->decrypt = modxtea_cipher_decrypt;
            break;
        default:
            free(key_data);
            free(cipher);
            return NULL;
    }

    return cipher;
}

// 销毁加密接口
void cipher_factory_destroy(cipher_interface_t* cipher) {
    if (cipher) {
        if (cipher->cleanup) {
            cipher->cleanup(cipher);
        }
        free(cipher);
    }
}