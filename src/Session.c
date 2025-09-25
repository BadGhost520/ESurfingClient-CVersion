//
// Created by bad_g on 2025/9/24.
//
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "headFiles/Session.h"
#include "headFiles/States.h"
#include "headFiles/cipher/Interface.h"

bool initialized = false;
cipher_interface_t* cipher = NULL;  // 全局加密实例

// 加密函数 (对应Kotlin: fun encrypt(text: String): String)
char* encrypt(const char* text) {
    if (!initialized || cipher == NULL) {
        printf("[CIPHER] Session not initialized or cipher is NULL\n");
        return NULL;
    }

    if (text == NULL) {
        printf("[CIPHER] Input text is NULL\n");
        return NULL;
    }

    char* ciphertext = NULL;
    int result = cipher->encrypt(cipher, text, &ciphertext);

    if (result != 0) {
        printf("[CIPHER] Encryption failed with error code: %d\n", result);
        return NULL;
    }

    return ciphertext;
}

// 解密函数 (对应Kotlin: fun decrypt(hex: String): String)
char* decrypt(const char* hex) {
    if (!initialized || cipher == NULL) {
        printf("[CIPHER] Session not initialized or cipher is NULL\n");
        return NULL;
    }

    if (hex == NULL) {
        printf("[CIPHER] Input hex is NULL\n");
        return NULL;
    }

    char* plaintext = NULL;
    int result = cipher->decrypt(cipher, hex, &plaintext);

    if (result != 0) {
        printf("[CIPHER] Decryption failed with error code: %d\n", result);
        return NULL;
    }

    return plaintext;
}

// 释放Session资源 (对应Kotlin: fun free())
void session_free(void) {
    initialized = false;
}

// 初始化加密组件 (对应CipherFactory.getInstance)
int init_cipher(const char* algo_id) {
    // 如果已经有cipher实例，先释放
    if (cipher != NULL) {
        cipher_factory_destroy(cipher);
        cipher = NULL;
    }

    // 使用crypto_factory创建加密实例
    cipher = cipher_factory_create(algo_id);
    if (cipher == NULL) {
        printf("[CIPHER] Failed to create cipher for algo_id: %s\n", algo_id);
        return 0; // 失败
    }
    return 1; // 成功
}

/**
 * 将十六进制字符串转换为二进制数据
 */
int hex_string_to_binary(const char* hex_str, size_t hex_len, unsigned char** binary_data, size_t* binary_len) {
    // 参数验证：在当前调用上下文中hex_str已经过NULL检查，但保留验证以确保函数的通用性
    if (hex_len == 0 || hex_len % 2 != 0) {
        return 0; // 十六进制字符串长度必须是偶数且非空
    }

    *binary_len = hex_len / 2;
    *binary_data = malloc(*binary_len);
    if (!*binary_data) {
        return 0;
    }

    for (size_t i = 0; i < *binary_len; i++) {
        char hex_byte[3] = {hex_str[i*2], hex_str[i*2+1], '\0'};
        char* endptr;

        // 修复警告：使用strtoul替代sscanf进行更安全的转换
        unsigned long byte_val = strtoul(hex_byte, &endptr, 16);

        // 检查转换是否成功：endptr应该指向字符串末尾，且值在有效范围内
        if (*endptr != '\0' || byte_val > 255) {
            free(*binary_data);
            *binary_data = NULL;
            return 0;
        }
        (*binary_data)[i] = (unsigned char)byte_val;
    }

    return 1;
}

/**
 * C语言版本的load函数
 * 对应Kotlin: private fun load(zsm: ByteArray): Boolean
 * 修复版本：正确处理服务器返回的十六进制字符串格式数据
 *
 * @param zsm 字节数组指针（可能包含十六进制字符串）
 * @return 1表示成功，0表示失败
 */
int load(const ByteArray* zsm) {
    printf("\n=== load_session 调试信息 (修复版) ===\n");

    // 输入验证
    if (zsm == NULL || zsm->data == NULL) {
        printf("错误: zsm为NULL或data为NULL\n");
        return 0;
    }

    printf("接收到的zsm数据长度: %zu\n", zsm->length);

    // 检查数据是否为十六进制字符串格式
    int is_hex_string = 1;
    for (size_t i = 0; i < zsm->length && i < 10; i++) {
        // 修复警告：使用unsigned char避免narrowing conversion
        unsigned char c = zsm->data[i];
        if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) {
            is_hex_string = 0;
            break;
        }
    }

    unsigned char* binary_data = NULL;
    size_t binary_len = 0;
    const unsigned char* data_to_parse = NULL;

    if (is_hex_string) {
        printf("检测到十六进制字符串格式，正在转换为二进制数据...\n");
        if (!hex_string_to_binary((const char*)zsm->data, zsm->length, &binary_data, &binary_len)) {
            printf("错误: 十六进制字符串转换失败\n");
            return 0;
        }
        data_to_parse = binary_data;
        printf("转换后的二进制数据长度: %zu\n", binary_len);
    } else {
        printf("检测到二进制数据格式，直接解析\n");
        data_to_parse = zsm->data;
        binary_len = zsm->length;
    }

    // 验证数据长度 (对应: if (zsm.size < 4))
    if (binary_len < 4) {
        printf("错误: 数据长度不足4字节 (实际: %zu)\n", binary_len);
        if (binary_data) free(binary_data);
        return 0;
    }

    // 解析头部信息 (对应: val header = zsm.sliceArray(0 until 3).decodeToString())
    char header[4] = {0};
    memcpy(header, data_to_parse, 3);
    header[3] = '\0'; // 确保字符串结尾
    printf("头部信息: '%s' (十六进制: %02X %02X %02X)\n", header, data_to_parse[0], data_to_parse[1], data_to_parse[2]);

    // 获取密钥长度 (对应: val keyLen = zsm[3])
    unsigned char key_len = data_to_parse[3];
    printf("密钥长度: %d\n", key_len);

    // 初始化位置指针 (对应: var pos = 4)
    size_t pos = 4;

    // 验证密钥长度是否合理 (对应: if (pos + keyLen > zsm.size))
    if (pos + key_len > binary_len) {
        printf("错误: 密钥长度超出数据范围 (需要: %zu, 可用: %zu)\n", pos + key_len, binary_len);
        if (binary_data) free(binary_data);
        return 0;
    }

    // 获取密钥数据 (对应: val key = zsm.sliceArray(pos until pos + keyLen).decodeToString())
    char* key = malloc(key_len + 1);
    if (key == NULL) {
        printf("错误: 无法分配密钥内存\n");
        if (binary_data) free(binary_data);
        return 0;
    }
    memcpy(key, data_to_parse + pos, key_len);
    key[key_len] = '\0';
    printf("密钥: '%s'\n", key);
    pos += key_len;

    // 验证算法ID长度字段 (对应: if (pos >= zsm.size))
    if (pos >= binary_len) {
        printf("错误: 无法读取算法ID长度字段 (位置: %zu, 数据长度: %zu)\n", pos, binary_len);
        free(key);
        if (binary_data) free(binary_data);
        return 0;
    }

    // 获取算法ID长度 (对应: val algoIdLen = zsm[pos])
    unsigned char algo_id_len = data_to_parse[pos];
    printf("算法ID长度: %d\n", algo_id_len);
    pos += 1;

    // 验证算法ID长度是否合理 (对应: if (pos + algoIdLen > zsm.size))
    if (pos + algo_id_len > binary_len) {
        printf("错误: 算法ID长度超出数据范围 (需要: %zu, 可用: %zu)\n", pos + algo_id_len, binary_len);
        free(key);
        if (binary_data) free(binary_data);
        return 0;
    }

    // 获取算法ID (对应: val algoId = zsm.sliceArray(pos until pos + algoIdLen).decodeToString())
    char* algo_id = malloc(algo_id_len + 1);
    if (algo_id == NULL) {
        printf("错误: 无法分配算法ID内存\n");
        free(key);
        if (binary_data) free(binary_data);
        return 0;
    }
    memcpy(algo_id, data_to_parse + pos, algo_id_len);
    algo_id[algo_id_len] = '\0';
    printf("算法ID: '%s'\n", algo_id);

    // 初始化加密器 (对应: cipher = CipherFactory.getInstance(algoId))
    printf("正在初始化加密器...\n");
    if (!init_cipher(algo_id)) {
        printf("错误: 无法初始化加密器\n");
        free(key);
        free(algo_id);
        if (binary_data) free(binary_data);
        return 0;
    }
    printf("加密器初始化成功\n");

    // 更新全局状态 (对应: States.algoId = algoId)
    // 释放旧的algoId内存（如果已分配）
    if (algoId != NULL) {
        free(algoId);
    }

    // 分配新内存并复制算法ID
    algoId = malloc(strlen(algo_id) + 1);
    if (algoId == NULL) {
        printf("错误: 无法分配全局algoId内存\n");
        free(key);
        free(algo_id);
        if (binary_data) free(binary_data);
        return 0;
    }
    strcpy(algoId, algo_id);
    printf("全局algoId已更新: '%s'\n", algoId);

    // 清理内存
    free(key);
    free(algo_id);
    if (binary_data) free(binary_data);

    printf("Session加载成功!\n");
    printf("========================\n\n");

    // 成功返回 (对应: return true)
    return 1;
}

void initialize(const ByteArray* zsm)
{
    printf("Initializing Session...\n");
    initialized = load(zsm);
}

bool isInitialized(void)
{
    return initialized;
}