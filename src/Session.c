//
// Created by bad_g on 2025/9/24.
//
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "headFiles/utils/ByteArray.h"
#include "headFiles/Session.h"
#include "headFiles/States.h"
#include "headFiles/cipher/cipher_interface.h"

bool initialized = false;
cipher_interface_t* cipher = NULL;  // 全局加密实例

const char* check1 = "B809531F-0007-4B5B-923B-4BD560398113";
const char* check2 = "F3974434-C0DD-4C20-9E87-DDB6814A1C48";

// 加密 (对应fun encrypt(text: String): String)
char* encrypt(const char* text)
{
    return cipher->encrypt(cipher, text);
}

// 解密 (对应fun encrypt(text: String): String)
char* decrypt(const char* text)
{
    return cipher->decrypt(cipher, text);
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
 * @return 1表示成功，0表示失败，2表示需要重新获取
 */
int load(const ByteArray* zsm) {
    char* key;
    char* algo_id;
    // printf("接收到的zsm数据长度: %zu\n", zsm->length);

    if (!zsm || !zsm->data || zsm->length == 0) {
        key = NULL;
        algo_id = NULL;
        return 0;
    }

    // 将字节数组转换为字符串
    char* str = (char*)malloc(zsm->length + 1);
    if (!str) {
        key = NULL;
        algo_id = NULL;
        return 0;
    }

    memcpy(str, zsm->data, zsm->length);
    str[zsm->length] = '\0';  // 添加字符串终止符

    // printf("原始字符串: %s\n", str);
    // printf("字符串长度: %zu\n", strlen(str));

    // 检查字符串长度是否足够
    if (strlen(str) < 4 + 38) {
        printf("错误: 字符串长度不足\n");
        free(str);
        key = NULL;
        algo_id = NULL;
        return 0;
    }

    // 提取 Key: 去掉左边4个字符，去掉右边38个字符
    size_t key_length = strlen(str) - 4 - 38;
    if (key_length <= 0) {
        printf("错误: Key长度计算错误\n");
        free(str);
        key = NULL;
        algo_id = NULL;
        return 0;
    }

    key = (char*)malloc(key_length + 1);
    if (key) {
        strncpy(key, str + 4, key_length);
        (key)[key_length] = '\0';
        // printf("提取的Key: %s\n", key);
        // printf("Key长度: %zu\n", key_length);
    }

    // 提取 algo_id: 从右往左数38个字符（去掉最后的']'）
    size_t total_length = strlen(str);
    if (total_length >= 38) {
        size_t algo_id_length = 36;  // 38个字符去掉最后的']'
        algo_id = (char*)malloc(algo_id_length + 1);
        if (algo_id) {
            // 从倒数第37个字符开始，取36个字符
            strncpy(algo_id, str + total_length - 37, algo_id_length);
            (algo_id)[algo_id_length] = '\0';
            // printf("提取的algo_id: %s\n", algo_id);
            if (!strcmp(algo_id, check1) || !strcmp(algo_id, check2))
            {
                free(key);
                free(algo_id);
                return 2;
            }
        }
    } else {
        algo_id = NULL;
        printf("错误: 字符串长度不足以提取algo_id\n");
        return 0;
    }
    free(str);
    printf("[Session.c/load] Algo ID: %s\n", algo_id);
    printf("[Session.c/load] Key: %s\n", key);

    // 初始化加密器 (对应: cipher = CipherFactory.getInstance(algoId))
    // printf("正在初始化加密器...\n");
    if (!init_cipher(algo_id)) {
        printf("错误: 无法初始化加密器\n");
        free(key);
        free(algo_id);
        return 0;
    }
    // printf("加密器初始化成功\n");

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
        return 0;
    }
    strcpy(algoId, algo_id);
    // printf("全局algoId已更新: '%s'\n", algoId);

    // 清理内存
    free(key);
    free(algo_id);

    // printf("Session加载成功!\n");

    // 成功返回 (对应: return true)
    return 1;
}

int initialize(const ByteArray* zsm)
{
    printf("[Session.c/initialize] 正在初始化 Session\n");
    switch (load(zsm))
    {
    case 0:
        initialized = false;
        return 0;
    case 1:
        initialized = true;
        return 1;
    case 2:
        return 2;
    default:
        return 0;
    }
}

bool isInitialized(void)
{
    return initialized;
}