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
 * C语言版本的load函数
 * 对应Kotlin: private fun load(zsm: ByteArray): Boolean
 *
 * @param zsm 字节数组指针
 * @return 1表示成功，0表示失败
 */
int load_session(const ByteArray* zsm) {
    printf("\n=== load_session 调试信息 ===\n");

    // 输入验证
    if (zsm == NULL || zsm->data == NULL) {
        printf("错误: zsm为NULL或data为NULL\n");
        return 0;
    }

    printf("接收到的zsm数据长度: %zu\n", zsm->length);

    // 验证数据长度 (对应: if (zsm.size < 4))
    if (zsm->length < 4) {
        printf("错误: 数据长度不足4字节 (实际: %zu)\n", zsm->length);
        return 0;
    }

    // 解析头部信息 (对应: val header = zsm.sliceArray(0 until 3).decodeToString())
    char header[4] = {0};
    memcpy(header, zsm->data, 3);
    header[3] = '\0'; // 确保字符串结尾
    printf("头部信息: '%s' (十六进制: %02X %02X %02X)\n", header, zsm->data[0], zsm->data[1], zsm->data[2]);

    // 获取密钥长度 (对应: val keyLen = zsm[3])
    unsigned char key_len = zsm->data[3];
    printf("密钥长度: %d\n", key_len);

    // 初始化位置指针 (对应: var pos = 4)
    size_t pos = 4;

    // 验证密钥长度是否合理 (对应: if (pos + keyLen > zsm.size))
    if (pos + key_len > zsm->length) {
        printf("错误: 密钥长度超出数据范围 (需要: %zu, 可用: %zu)\n", pos + key_len, zsm->length);
        return 0;
    }

    // 获取密钥数据 (对应: val key = zsm.sliceArray(pos until pos + keyLen).decodeToString())
    char* key = malloc(key_len + 1);
    if (key == NULL) {
        printf("错误: 无法分配密钥内存\n");
        return 0;
    }
    memcpy(key, zsm->data + pos, key_len);
    key[key_len] = '\0';
    printf("密钥: '%s'\n", key);
    pos += key_len;

    // 验证算法ID长度字段 (对应: if (pos >= zsm.size))
    if (pos >= zsm->length) {
        printf("错误: 无法读取算法ID长度字段 (位置: %zu, 数据长度: %zu)\n", pos, zsm->length);
        free(key);
        return 0;
    }

    // 获取算法ID长度 (对应: val algoIdLen = zsm[pos])
    unsigned char algo_id_len = zsm->data[pos];
    printf("算法ID长度: %d\n", algo_id_len);
    pos += 1;

    // 验证算法ID长度是否合理 (对应: if (pos + algoIdLen > zsm.size))
    if (pos + algo_id_len > zsm->length) {
        printf("错误: 算法ID长度超出数据范围 (需要: %zu, 可用: %zu)\n", pos + algo_id_len, zsm->length);
        free(key);
        return 0;
    }

    // 获取算法ID (对应: val algoId = zsm.sliceArray(pos until pos + algoIdLen).decodeToString())
    char* algo_id = malloc(algo_id_len + 1);
    if (algo_id == NULL) {
        printf("错误: 无法分配算法ID内存\n");
        free(key);
        return 0;
    }
    memcpy(algo_id, zsm->data + pos, algo_id_len);
    algo_id[algo_id_len] = '\0';
    printf("算法ID: '%s'\n", algo_id);

    // 初始化加密器 (对应: cipher = CipherFactory.getInstance(algoId))
    printf("正在初始化加密器...\n");
    if (!init_cipher(algo_id)) {
        printf("错误: 无法初始化加密器\n");
        free(key);
        free(algo_id);
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
        return 0;
    }
    strcpy(algoId, algo_id);
    printf("全局algoId已更新: '%s'\n", algoId);

    // 清理内存
    free(key);
    free(algo_id);

    printf("Session加载成功!\n");
    printf("========================\n\n");

    // 成功返回 (对应: return true)
    return 1;
}

void SessionInitialize(const ByteArray* zsm)
{
    printf("Initializing Session...\n");
    if (load_session(zsm))
    {
        initialized = true;
    }
}

bool isInitialized(void)
{
    return initialized;
}