#include "clients/dialer/DialerInternal.h"

#include "clients/net/NetClient.h"

#include "cipher/CipherInterface.h"

#include "states/States.h"

#include "utils/PlatformUtils.h"
#include "utils/Logger.h"

#include <ctype.h>
#include <stdio.h>

bool get_ticket()
{
    LOG_DEBUG("get_ticket 函数入口检查, 使用配置: %" PRIu8 ", 下标: %" PRIu8, g_prog_status[tl_thread_idx].login_cfg.idx, tl_thread_idx);

    const char* xml = create_xml_payload(GET_TICKET); // 创建 get_ticket 用的 xml
    if (xml == NULL)
    {
        LOG_ERROR("创建获取 Ticket XML 失败");
        return false;
    }

    char* encrypt = session_encrypt(xml); // 加密 xml
    if (encrypt == NULL)
    {
        LOG_ERROR("加密获取 Ticket XML 失败");
        return false;
    }
    LOG_VERBOSE("发送加密获取 ticket 内容: %s", encrypt);

    const curl_resp_t resp = post(g_prog_status[tl_thread_idx].auth_cfg.ticket_url, encrypt); // 向 ticket_url 发送加密内容
    free(encrypt);
    if (resp.http_code != HTTP_OK || resp.body_size == 0 || resp.body_data == NULL)
    {
        LOG_ERROR("获取 Ticket 响应失败");
        free(resp.body_data);
        return false;
    }
    LOG_VERBOSE("获取 Ticket 响应内容: %s", resp.body_data);

    char* decrypt = session_decrypt(resp.body_data); // 解密响应内容
    free(resp.body_data);
    if (decrypt == NULL)
    {
        LOG_ERROR("解密 Ticket 内容失败");
        return false;
    }

    char* parsed_ticket = xml_parser(decrypt, "ticket"); // 获取 ticket
    free(decrypt);
    if (parsed_ticket == NULL)
    {
        LOG_ERROR("解析 Ticket 失败");
        return false;
    }
    snprintf(g_prog_status[tl_thread_idx].auth_cfg.ticket, TICKET_LEN, "%s", safe_str(parsed_ticket)); // 将 ticket 填入认证配置中
    LOG_INFO("Ticket: %s", g_prog_status[tl_thread_idx].auth_cfg.ticket);
    free(parsed_ticket);
    return true;
}

static bool is_uuid_text(const uint8_t* data, size_t length)
{
    static const uint8_t hyphen_pos[] = {8, 13, 18, 23};
    unsigned hyphen_i = 0;

    if (data == NULL || length != 36)
    {
        return false;
    }

    for (size_t i = 0; i < 36; i++)
    {
        if (hyphen_i < 4 && i == hyphen_pos[hyphen_i])
        {
            if (data[i] != '-')
            {
                return false;
            }
            hyphen_i++;
            continue;
        }
        if (!isxdigit(data[i]))
        {
            return false;
        }
    }
    return true;
}

static void uuid_to_upper(char* dst, const uint8_t* src)
{
    for (size_t i = 0; i < 36; i++)
    {
        dst[i] = (char)toupper(src[i]);
    }
    dst[36] = '\0';
}

static bool read_zsm_pascal_string(const uint8_t* data, size_t length, size_t* offset, const uint8_t** out, size_t* out_len)
{
    if (data == NULL || offset == NULL || *offset >= length)
    {
        return false;
    }
    const uint8_t str_len = data[*offset];
    (*offset)++;
    if (*offset + str_len > length)
    {
        return false;
    }
    *out = data + *offset;
    *out_len = str_len;
    *offset += str_len;
    return true;
}

static bool extract_algo_id_from_zsm(const bytes_t zsm, char* algo_id)
{
    size_t offset;
    const uint8_t* str1 = NULL;
    const uint8_t* str2 = NULL;
    size_t str1_len = 0;
    size_t str2_len = 0;

    if (zsm.data == NULL || algo_id == NULL || zsm.len < 5)
    {
        return false;
    }

    offset = 3;
    if (read_zsm_pascal_string(zsm.data, zsm.len, &offset, &str1, &str1_len)
        && read_zsm_pascal_string(zsm.data, zsm.len, &offset, &str2, &str2_len))
    {
        if (is_uuid_text(str2, str2_len))
        {
            uuid_to_upper(algo_id, str2);
            return true;
        }
        if (is_uuid_text(str1, str1_len))
        {
            uuid_to_upper(algo_id, str1);
            return true;
        }
    }

    size_t end = zsm.len;
    while (end > 0)
    {
        const unsigned char c = zsm.data[end - 1];
        if (c == '\n' || c == '\r' || c == '\0' || c == ' ' || c == '\t')
        {
            end--;
            continue;
        }
        break;
    }
    if (end >= 36 && is_uuid_text(zsm.data + (end - 36), 36))
    {
        uuid_to_upper(algo_id, zsm.data + (end - 36));
        return true;
    }
    return false;
}

bool load_cipher(const bytes_t zsm)
{
    char algo_id[ALGO_ID_LEN];
    const uint8_t chn = g_prog_status[tl_thread_idx].login_cfg.chn;
    const bool ios_module = looks_like_ios_zsm(zsm.data, zsm.len);

    LOG_DEBUG("load 函数入口检查, 使用配置: %" PRIu8 ", 下标: %" PRIu8, g_prog_status[tl_thread_idx].login_cfg.idx, tl_thread_idx);
    LOG_INFO("当前通道: %" PRIu8 ", ZSM 长度: %zu, 动态 ZSM 模块: %s", chn, zsm.len, ios_module ? "是" : "否");
    if (zsm.data == NULL || zsm.len == 0) // 检查 zsm 数据是否为空, 为空则返回 false
    {
        LOG_ERROR("无效的 zsm 数据");
        return false;
    }

    /**
     * iOS PacketTunnel / macOS GDCV 的 ZSM 都是 TEA+LZMA 后的 JS 模块,
     * 头部 UUID 只是模块 ID, 不在 Android/Linux CipherFactory 里.
     * 通道号只决定 UA/主机名, 不决定密钥解包方式.
     */
    if (chn == 4 || chn == 5 || ios_module)
    {
        if (chn != 4 && chn != 5)
        {
            LOG_WARN("通道不是 iOS/macOS, 但 ticket 返回了动态 ZSM, 按动态密钥解包, UA 不变");
        }
        if (init_ios_cipher_from_zsm(zsm.data, zsm.len, algo_id) == false)
        {
            LOG_ERROR("无法按动态 ZSM 解包出密钥 (长度 %zu, 通道 %" PRIu8 ")", zsm.len, chn);
            if (chn == 4 || chn == 5)
            {
                return false;
            }
            LOG_WARN("动态 ZSM 解包失败, 回退到 CipherFactory");
        }
        else
        {
            snprintf(g_prog_status[tl_thread_idx].auth_cfg.algo_id, ALGO_ID_LEN, "%s", safe_str(algo_id));
            LOG_DEBUG("全局 AlgoID 已更新: %s", g_prog_status[tl_thread_idx].auth_cfg.algo_id);
            return true;
        }
    }

    if (extract_algo_id_from_zsm(zsm, algo_id) == false)
    {
        LOG_ERROR("无法从 ZSM 中提取 Algo-ID (长度 %zu)", zsm.len);
        return false;
    }
    LOG_INFO("Algo ID: %s", algo_id);

    if (init_cipher(algo_id) == false)
    {
        LOG_WARN("CipherFactory 没有 Algo-ID %s, 尝试按动态 ZSM 解包", algo_id);
        if (init_ios_cipher_from_zsm(zsm.data, zsm.len, algo_id))
        {
            snprintf(g_prog_status[tl_thread_idx].auth_cfg.algo_id, ALGO_ID_LEN, "%s", safe_str(algo_id));
            LOG_DEBUG("全局 AlgoID 已更新: %s", g_prog_status[tl_thread_idx].auth_cfg.algo_id);
            return true;
        }
        LOG_ERROR("未知 Algo-ID: %s, 当前通道没有对应密钥", algo_id);
        return false;
    }
    snprintf(g_prog_status[tl_thread_idx].auth_cfg.algo_id, ALGO_ID_LEN, "%s", safe_str(algo_id)); // 将 algo_id 填入认证配置中
    LOG_DEBUG("全局 AlgoID 已更新: %s", g_prog_status[tl_thread_idx].auth_cfg.algo_id);
    return true;
}
