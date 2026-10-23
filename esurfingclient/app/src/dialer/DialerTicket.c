#include "DialerInternal.h"

#include "net/NetClient.h"

#include "cipher/CipherInterface.h"

#include "core/States.h"

#include "dialer/ErrorCode.h"


#include "utils/text/TextUtils.h"
#include "utils/log/Logger.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

/* 已在两台服务器上稳定返回 Error-Code=1 的 Windows Algo-ID */
static const char* const WINDOWS_BAD_ALGO_IDS[] = {
    "014AC5DB-0B7C-4F05-B21E-55C2535B3A01",
    "04DB9EC7-155A-4482-B6A4-1F37A3B905A4",
    "054DDD03-911E-49F5-89D6-EFBF5055FBFF",
    "083B005A-7ACA-419A-AC00-6929C0AADB55",
    "08BDB042-5D25-4397-875F-357E9F7700C8",
    "10FD0A56-0044-45F0-A342-7CE242B1A427",
    "14D50A01-FB30-4ED0-B287-96286E3091C1",
    "14F3C2A4-19E9-4C4D-A5C1-E5A316C884F0",
    "1B652785-1153-4741-8A1C-2D9E752BE0E5",
    "2C2C4070-6644-4E84-BDBD-1760A2DA875C",
    "2E86DCEE-4E28-4AC2-A3FB-19F68115141E",
    "32C8C121-2467-42C4-A09D-1B0BA754D18F",
    "37620C35-7A97-4A4F-9EF3-822DD38FE211",
    "376412D4-ACA3-4452-8A79-4589B3817760",
    "3FBF4B5E-972D-43EF-A454-46E6BE15B329",
    "42184E84-A844-486E-83C3-4CB9FD874D67",
    "52271930-386B-4EC7-B5E9-7DEF7C1C11A8",
    "578FDB96-AF47-461D-AC36-D08EADF10130",
    "58FD3FCA-C8BC-4547-9DA5-6FAEE884F26E",
    "6B4D085A-17D5-4404-94BE-FD5FED11007F",
    "6BB82AD9-D331-4BE8-BC52-C1F56E351C1C",
    "6DF91D90-3324-4064-BBA8-EFF3820BDF1E",
    "70CDBBAB-C9C9-49DE-A8D9-068579EAFFD0",
    "76C9B8BC-FCAA-445A-93CC-D22B9413159D",
    "76CD0763-8C5F-49B0-85CD-549BB32886A2",
    "78AD4214-E71F-421A-A13B-C65EC32890C3",
    "79F1CC4F-A213-4DE0-AAED-A276D2E1F21F",
    "7DE2123D-3868-4D5E-86A1-687FE2683476",
    "7F224CA8-0DF5-47C6-9195-2D335E31D882",
    "895E36A7-0175-4EF2-A00D-7F3482D70D72",
    "9E6620CB-628D-49CA-80DD-9E6BCA401A31",
    "A1DFF4B8-770B-4EB3-9F79-719F2196E315",
    "A7BBDBD5-DB4C-46A8-91E8-E4C8003C8CDD",
    "ABA03E9C-E7E3-4AD0-94F9-4224DAA975C4",
    "AF69A651-96D0-4B88-B388-644CB6822159",
    "BFAC1582-AA39-4CBC-AD0E-6D37ACDA369B",
    "C8E20AA2-0254-4C36-ACF1-EF4779934D08",
    "CAC870CA-C03D-475A-8CB0-F4E9C80B9153",
    "DF011932-AFC2-447D-948D-A21EFA7411B8",
    "DF4FF2CD-3387-4377-9A91-E90E05081073",
    "DF92372F-EF9B-44FC-ABB4-EC5625DE60B6",
    "E2BC8BAC-D7E0-4DAD-9400-789C63D6A0DC",
    "E51E14F1-BCB1-45F4-8FF6-3992CA3FCD05",
    "E9747EF1-6849-4F87-957B-3B93E6A90393",
    "EA85A478-CDD9-483C-98C4-4CEE1347C9B9",
    "F1D8F312-EADC-4803-9B47-9C7684B2156F",
    "F6B5CFB0-17FD-453C-9791-B2887E2C9859",
    "FE8F87FC-726F-4303-B15D-88A9F5B63174",
    "FEE5E4DA-50A0-4ECD-8392-92E09CB1B775",
    "FF5F345F-B580-43DF-9ED3-D92FAE586133",
};

static bool is_bad_algo_id(const char* algo_id)
{
    if (algo_id == NULL) return false;
    for (size_t i = 0; i < sizeof(WINDOWS_BAD_ALGO_IDS) / sizeof(WINDOWS_BAD_ALGO_IDS[0]); i++)
    {
        if (strcmp(WINDOWS_BAD_ALGO_IDS[i], algo_id) == 0) return true;
    }
    return false;
}

bool get_ticket(void)
{
    LOG_DEBUG("get_ticket 函数入口检查, 使用配置: %" PRIu8 ", 下标: %" PRIu8, g_prog_status[tl_thread_idx].login_cfg.idx, tl_thread_idx);

    char* decrypt = dialer_request_ticket(GET_TICKET, g_prog_status[tl_thread_idx].auth_cfg.ticket_url, "获取 Ticket");
    if (decrypt == NULL) return false;

    char* parsed_ticket = xml_parser(decrypt, "ticket"); // 获取 ticket
    free(decrypt);
    if (parsed_ticket == NULL)
    {
        LOG_ERROR("解析 Ticket 失败");
        return false;
    }
    snprintf(g_prog_status[tl_thread_idx].auth_cfg.ticket, TICKET_LEN, "%s", safe_str(parsed_ticket)); // 将 ticket 填入认证配置中
    LOG_DEBUG("Ticket: %s", g_prog_status[tl_thread_idx].auth_cfg.ticket);
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

    if (zsm.data == NULL || zsm.len == 0)
    {
        LOG_ERROR("无效的 zsm 数据");
        return false;
    }

    if (extract_algo_id_from_zsm(zsm, algo_id) == false)
    {
        LOG_DEBUG("ZSM 中未解析出 Algo-ID (长度 %zu), 重新获取", zsm.len);
        return false;
    }
    LOG_DEBUG("ZSM Algo-ID: %s", algo_id);

    if (is_bad_algo_id(algo_id))
    {
        LOG_DEBUG("Algo-ID %s 已知 Error-Code=1, 直接丢弃并重新获取", algo_id);
        return false;
    }

    if (cipher_supported(algo_id) == false)
    {
        LOG_DEBUG("Algo-ID %s 不在硬编码表, 重新获取", algo_id);
        return false;
    }

    if (init_cipher(algo_id) == false)
    {
        LOG_ERROR("初始化 Algo-ID 失败: %s", algo_id);
        return false;
    }

    snprintf(g_prog_status[tl_thread_idx].auth_cfg.algo_id, ALGO_ID_LEN, "%s", safe_str(algo_id));
    LOG_DEBUG("全局 AlgoID 已更新: %s", g_prog_status[tl_thread_idx].auth_cfg.algo_id);
    return true;
}
