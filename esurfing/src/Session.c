//
// Created by bad_g on 2025/9/24.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "headFiles/utils/PlatformUtils.h"
#include "headFiles/Session.h"
#include "headFiles/States.h"
#include "headFiles/cipher/CipherInterface.h"
#include "headFiles/utils/Logger.h"

void sessionFree()
{
    LOG_DEBUG("清除会话初始化状态");
    isInitialized = 0;
}

int load(const ByteArray* zsm)
{
    char* key = NULL;
    char* algo_id = NULL;
    LOG_DEBUG("接收到的 zsm 数据长度: %zu", zsm->length);
    if (!zsm->data || zsm->length == 0)
    {
        LOG_ERROR("无效的 zsm 数据");
        return 0;
    }
    char* str = malloc(zsm->length + 1);
    if (!str)
    {
        LOG_ERROR("分配 zsm 字符串数据内存失败");
        return 0;
    }
    memcpy(str, zsm->data, zsm->length);
    str[zsm->length] = '\0';
    LOG_DEBUG("原始字符串: %s", str);
    LOG_DEBUG("字符串长度: %zu", strlen(str));
    if (strlen(str) < 4 + 38)
    {
        LOG_ERROR("字符串长度不足");
        free(str);
        return 0;
    }
    const size_t key_length = strlen(str) - 4 - 38;
    if (key_length <= 0)
    {
        LOG_ERROR("Key 长度计算错误");
        free(str);
        return 0;
    }
    key = (char*)malloc(key_length + 1);
    if (!key)
    {
        LOG_ERROR("分配 Key 内存失败");
        free(str);
        return 0;
    }
    strncpy(key, str + 4, key_length);
    key[key_length] = '\0';
    LOG_DEBUG("提取 Key: %s", key);
    LOG_DEBUG("Key 长度: %zu", key_length);
    
    const size_t total_length = strlen(str);
    if (total_length >= 38)
    {
        const size_t algo_id_length = 36;
        algo_id = (char*)malloc(algo_id_length + 1);
        if (!algo_id)
        {
            LOG_ERROR("分配 AlgoId 内存失败");
            free(key);
            free(str);
            return 0;
        }
        strncpy(algo_id, str + total_length - 37, algo_id_length);
        algo_id[algo_id_length] = '\0';
        LOG_DEBUG("提取 AlgoId: %s", algo_id);
    }
    else
    {
        LOG_ERROR("字符串长度不足以提取 AlgoId");
        free(key);
        free(str);
        return 0;
    }
    free(str);
    LOG_INFO("Algo ID: %s", algo_id);
    LOG_INFO("Key: %s", key);
    if (!initCipher(algo_id))
    {
        free(key);
        free(algo_id);
        return 0;
    }
    if (algoId != NULL) free(algoId);
    algoId = malloc(strlen(algo_id) + 1);
    if (algoId == NULL)
    {
        LOG_ERROR("分配全局 AlgoId 内存失败");
        free(key);
        free(algo_id);
        return 0;
    }
    strcpy(algoId, algo_id);
    LOG_DEBUG("全局 AlgoId 已更新: '%s'", algoId);
    free(key);
    free(algo_id);
    LOG_DEBUG("初始化会话成功");
    return 1;
}

void initialize(const ByteArray* zsm)
{
    LOG_DEBUG("开始初始化会话");
    if (load(zsm))
    {
        isInitialized = 1;
    }
    else
    {
        isInitialized = 0;
    }
}