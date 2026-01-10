#include <stdlib.h>
#include <string.h>

#include "headFiles/cipher/CipherInterface.h"
#include "headFiles/utils/PlatformUtils.h"
#include "headFiles/utils/Logger.h"
#include "headFiles/DialerClient.h"
#include "headFiles/Session.h"
#include "headFiles/States.h"

static InitStatus load(const ByteArray zsm)
{
    LOG_DEBUG("接收到的 zsm 数据长度: %zu", zsm.length);
    if (!zsm.data || zsm.length == 0)
    {
        LOG_ERROR("无效的 zsm 数据");
        return INIT_FAILURE;
    }
    char str[zsm.length + 1];
    memcpy(str, zsm.data, zsm.length);
    str[zsm.length] = '\0';
    const size_t length = strlen(str);
    LOG_DEBUG("原始字符串: %s", str);
    LOG_DEBUG("字符串长度: %zu", length);
    if (length < 4 + 38)
    {
        LOG_ERROR("字符串长度不足");
        return INIT_FAILURE;
    }
    char algo_id[ALGO_ID_LENGTH];
    memcpy(algo_id, str + length - 37, ALGO_ID_LENGTH - 1);
    LOG_INFO("Algo ID: %s", algo_id);
    if (!initCipher(algo_id))
    {
        LOG_ERROR("初始化加解密工厂失败");
        return INIT_FAILURE;
    }
    LOG_DEBUG("全局 AlgoID 已更新: '%s'", algo_id);
    snprintf(thread_status[thread_local_args.thread_index].dialer_context.auth_config.algo_id, ALGO_ID_LENGTH, "%s", algo_id);
    return INIT_SUCCESS;
}

void initialize(const ByteArray zsm)
{
    LOG_DEBUG("开始初始化会话");
    if (load(zsm) == INIT_SUCCESS)
    {
        LOG_DEBUG("初始化会话成功");
        thread_status[thread_local_args.thread_index].dialer_context.runtime_status.is_initialized = 1;
    }
    else
    {
        LOG_DEBUG("初始化会话失败");
        thread_status[thread_local_args.thread_index].dialer_context.runtime_status.is_initialized = 0;
    }
}

void freeSession()
{
    LOG_DEBUG("清除会话初始化状态");
    cipherFactoryDestroy();
    thread_status[thread_local_args.thread_index].dialer_context.runtime_status.is_initialized = 0;
}