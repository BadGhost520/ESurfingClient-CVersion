#include "cipher/CipherInterface.h"
#include "cipher/CipherUtils.h"

#include "states/States.h"

#include "utils/Logger.h"

#include <7z/LzmaDec.h>

/*
 * Windows 通道 (UA: CCTP/WinSVR5/1068) 的 ZSM 解包.
 *
 * 容器与 iOS 相同, 但 TEA 密钥取自 ESurfingSvr.exe:0x4156B0 dispatcher 的
 * version==2 表 (k1 = base+0x00, k2 = base+0x10, 与 ZXM 容器顺序相反),
 * 明文是原生 PE 模块. 模块内嵌 256 字节窗口自描述 key/iv:
 *     t[0xFE] = key_off   t[0xFB] = key_len   t[0xF9] = iv_len
 * 窗口基址硬编码在代码里, 扫 .text 的 "mov al, [W+0xFF]" 定位.
 */

#define WIN_ZSM_TEA_KEY "|iKFMm;FZjI6oItGmvm536xR8MJ-5C89"
#define WIN_ZSM_TEA_DELTA 0x61C88647u
#define WIN_ZSM_MAX_UNPACKED 0x8000000u
#define WIN_KEY_MAX 48u
#define WIN_IV_MAX 16u

static void* win_lzma_alloc(const ISzAllocPtr p, const size_t size)
{
    (void)p;
    return size ? malloc(size) : NULL;
}

static void win_lzma_free(const ISzAllocPtr p, void* address)
{
    (void)p;
    free(address);
}

static const ISzAlloc g_win_lzma_alloc = { .Alloc = win_lzma_alloc, .Free = win_lzma_free };

static bool win_copy_uuid(char* dst, const uint8_t* src, const size_t len)
{
    if (dst == NULL || src == NULL || len != 36)
    {
        return false;
    }
    for (size_t i = 0; i < 36; i++)
    {
        const unsigned char c = src[i];
        const int is_dash = (i == 8 || i == 13 || i == 18 || i == 23) && c == '-';
        if (!isxdigit(c) && !is_dash)
        {
            return false;
        }
        dst[i] = (char)toupper(c);
    }
    dst[36] = '\0';
    return true;
}

static void win_tea_decrypt_block(const uint8_t key16[16], uint8_t block[8])
{
    uint32_t k[4];

    for (uint8_t i = 0; i < 4; i++)
    {
        k[i] = bytes_2_uint32_le(key16 + (size_t)i * 4);
    }
    uint32_t v0 = bytes_2_uint32_le(block);
    uint32_t v1 = bytes_2_uint32_le(block + 4);
    uint32_t sum = WIN_ZSM_TEA_DELTA * (uint32_t)(-32);
    do
    {
        v1 -= ((v0 << 4) ^ (v0 >> 5)) + (k[(sum >> 11) & 3] + (v0 ^ sum));
        sum += WIN_ZSM_TEA_DELTA;
        v0 -= k[sum & 3] + (v1 ^ sum) + ((v1 << 4) ^ (v1 >> 5));
    } while (sum != 0);
    uint32_2_bytes_le(v0, block);
    uint32_2_bytes_le(v1, block + 4);
}

static void win_tea_decrypt_buffer(uint8_t* data, const size_t length)
{
    static const uint8_t key[] = WIN_ZSM_TEA_KEY;

    for (size_t off = 0; off < length; off += 8)
    {
        win_tea_decrypt_block(key, data + off);
        win_tea_decrypt_block(key + 16, data + off);
    }
}

static uint32_t win_rd32(const uint8_t* p)
{
    return bytes_2_uint32_le(p);
}

static uint16_t win_rd16(const uint8_t* p)
{
    return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}

typedef struct
{
    size_t va;
    size_t vsize;
    size_t rawsize;
    size_t raw;
    bool is_text;
} win_section_t;

static bool win_locate_key(const uint8_t* pe, const size_t len,
                           uint8_t key_out[WIN_KEY_MAX], size_t* key_len_out,
                           uint8_t iv_out[WIN_IV_MAX], size_t* iv_len_out)
{
    win_section_t secs[16];
    size_t nsec = 0;

    if (pe == NULL || len < 0x40 || pe[0] != 'M' || pe[1] != 'Z')
    {
        LOG_ERROR("Windows ZSM 明文不是 PE");
        return false;
    }
    const size_t pe_off = win_rd32(pe + 0x3C);
    if (pe_off + 0x18 > len || memcmp(pe + pe_off, "PE\0\0", 4) != 0)
    {
        LOG_ERROR("Windows ZSM PE 头非法");
        return false;
    }
    const size_t nsec_raw = win_rd16(pe + pe_off + 6);
    const size_t opt_size = win_rd16(pe + pe_off + 20);
    const size_t sec_off = pe_off + 24 + opt_size;
    const uint32_t image_base = win_rd32(pe + pe_off + 24 + 28);

    for (size_t i = 0; i < nsec_raw && nsec < 16; i++)
    {
        const size_t o = sec_off + i * 40;
        if (o + 40 > len)
        {
            break;
        }
        win_section_t s;
        s.vsize = win_rd32(pe + o + 8);
        s.va = win_rd32(pe + o + 12);
        s.rawsize = win_rd32(pe + o + 16);
        s.raw = win_rd32(pe + o + 20);
        s.is_text = (memcmp(pe + o, ".text", 5) == 0);
        const size_t sz = s.vsize > s.rawsize ? s.vsize : s.rawsize;
        if (s.raw + sz > len)
        {
            if (s.raw >= len)
            {
                continue;
            }
            s.vsize = s.rawsize = len - s.raw;
        }
        secs[nsec++] = s;
    }

    const uint8_t* win = NULL;

    for (size_t i = 0; i < nsec && win == NULL; i++)
    {
        if (!secs[i].is_text)
        {
            continue;
        }
        const uint8_t* t = pe + secs[i].raw;
        const size_t tlen = secs[i].vsize > secs[i].rawsize ? secs[i].vsize : secs[i].rawsize;
        for (size_t k = 0; k + 5 <= tlen; k++)
        {
            if (t[k] != 0xA0)
            {
                continue;
            }
            const uint32_t addr = win_rd32(t + k + 1);
            if (addr < image_base)
            {
                continue;
            }
            const size_t rva = (size_t)(addr - image_base);
            size_t off = 0;
            bool ok = false;
            for (size_t j = 0; j < nsec; j++)
            {
                if (rva >= secs[j].va && rva < secs[j].va + secs[j].vsize)
                {
                    off = secs[j].raw + (rva - secs[j].va);
                    ok = true;
                    break;
                }
            }
            if (!ok || off < 0xFF || off + 1 > len)
            {
                continue;
            }
            const size_t W = off - 0xFF;
            if (W + 256 > len)
            {
                continue;
            }
            const uint8_t* cand = pe + W;
            const size_t kl = cand[0xFB];
            const size_t il = cand[0xF9];
            const size_t ko = cand[0xFE];
            if ((kl == 16 || kl == 24 || kl == 32 || kl == 48) &&
                (il == 0 || il == 8 || il == 16) &&
                ko + kl + il <= 256)
            {
                win = cand;
                break;
            }
        }
    }

    if (win == NULL)
    {
        LOG_ERROR("Windows ZSM 未定位到密钥窗口");
        return false;
    }

    const size_t key_len = win[0xFB];
    const size_t iv_len = win[0xF9];
    const size_t key_off = win[0xFE];

    memcpy(key_out, win + key_off, key_len);
    if (iv_len)
    {
        memcpy(iv_out, win + key_off + key_len, iv_len);
    }
    *key_len_out = key_len;
    *iv_len_out = iv_len;
    LOG_INFO("Windows ZSM 密钥材料: key_len=%zu iv_len=%zu", key_len, iv_len);
    return true;
}

static void win_pad_copy(uint8_t* dst, const size_t dst_len, const uint8_t* src, const size_t src_len)
{
    memset(dst, 0, dst_len);
    if (src != NULL && src_len)
    {
        memcpy(dst, src, src_len < dst_len ? src_len : dst_len);
    }
}

/* 按 (key_len, iv_len) 选 cdy 类型; 算法实现复用仓库已有 create_* */
static int8_t win_cdy_from_lengths(const size_t key_len, const size_t iv_len)
{
    if (key_len == 32 && iv_len == 16) return 2; /* 双层 AES-128-CBC */
    if (key_len == 48 && iv_len == 0)  return 5; /* 三层改 TEA-ECB */
    if (key_len == 48 && iv_len == 8)  return 6; /* 三层改 TEA-CBC */
    if (key_len == 32 && iv_len == 0)  return 1; /* 双层 AES-128-ECB */
    if (key_len == 16 && iv_len == 16) return 8; /* SM4-CBC */
    if (key_len == 16 && iv_len == 0)  return 7; /* SM4-ECB */
    return -1;
}

static cipher_interface_t* win_create_cipher(const int8_t type,
                                             const uint8_t* key, const size_t key_len,
                                             const uint8_t* iv, const size_t iv_len)
{
    uint8_t k[48];
    uint8_t v[16];

    switch (type)
    {
    case 1:
        win_pad_copy(k, 32, key, key_len);
        return create_aes_double_ecb_android_cipher(k);
    case 2:
        win_pad_copy(k, 32, key, key_len);
        win_pad_copy(v, 16, iv, iv_len);
        return create_aes_double_cbc_android_cipher(k, v);
    case 5:
        win_pad_copy(k, 48, key, key_len);
        return create_tea_triple_ecb_android_cipher(k);
    case 6:
        win_pad_copy(k, 48, key, key_len);
        win_pad_copy(v, 8, iv, iv_len);
        return create_tea_triple_cbc_android_cipher(k, v);
    case 7:
        win_pad_copy(k, 16, key, key_len);
        return create_sm4_variant_ecb_android_cipher(k);
    case 8:
        win_pad_copy(k, 16, key, key_len);
        win_pad_copy(v, 16, iv, iv_len);
        return create_sm4_variant_cbc_android_cipher(k, v);
    default:
        return NULL;
    }
}

bool init_win_cipher_from_zsm(const uint8_t* data, const size_t length, char* algo_id_out)
{
    uint8_t* cipher = NULL;
    uint8_t* unpacked = NULL;
    uint8_t key[WIN_KEY_MAX];
    uint8_t iv[WIN_IV_MAX];

    if (data == NULL || length < 15)
    {
        return false;
    }

    size_t offset = 3;
    const uint8_t len1 = data[offset++];
    if (offset + len1 + 1 > length)
    {
        LOG_ERROR("Windows ZSM str1 越界");
        return false;
    }
    offset += len1;
    const uint8_t len2 = data[offset++];
    if (offset + len2 > length)
    {
        LOG_ERROR("Windows ZSM str2 越界");
        return false;
    }
    if (algo_id_out)
    {
        if (win_copy_uuid(algo_id_out, data + offset, len2))
        {
            LOG_INFO("Windows ZSM Algo-ID: %s", algo_id_out);
        }
        else
        {
            snprintf(algo_id_out, ALGO_ID_LEN, "00000000-0000-0000-0000-000000000000");
        }
    }
    offset += len2;

    const size_t remain = length - offset;
    if (remain <= 9)
    {
        LOG_ERROR("Windows ZSM 长度不足");
        return false;
    }
    const uint8_t* props = data + offset;
    const uint32_t packed = win_rd32(props + 5);
    const uint32_t unpacked_size = packed & 0x0FFFFFFFu;
    if ((packed >> 28) != 2 || unpacked_size == 0 || unpacked_size > WIN_ZSM_MAX_UNPACKED)
    {
        LOG_ERROR("Windows ZSM packed 头非法: 0x%08X", packed);
        return false;
    }

    const size_t cipher_len = remain - 9;
    cipher = s_calloc(1, cipher_len + 8);
    memcpy(cipher, props + 9, cipher_len);
    win_tea_decrypt_buffer(cipher, cipher_len);

    const uint8_t pad = cipher_len ? cipher[cipher_len - 1] : 0;
    if (pad > cipher_len)
    {
        LOG_ERROR("Windows ZSM TEA 填充长度非法: %u", pad);
        s_free(cipher);
        return false;
    }

    SizeT src_len = cipher_len - pad;
    SizeT dest_len = (SizeT)unpacked_size;
    unpacked = s_calloc(1, (size_t)unpacked_size + 1);
    ELzmaStatus status;
    const SRes lzma_ret = LzmaDecode(unpacked, &dest_len, cipher, &src_len, props, 5,
                                     LZMA_FINISH_ANY, &status, &g_win_lzma_alloc);
    s_free(cipher);
    cipher = NULL;

    if (lzma_ret != SZ_OK || dest_len != (SizeT)unpacked_size)
    {
        LOG_ERROR("Windows ZSM LZMA 解压失败: %d", lzma_ret);
        s_free(unpacked);
        return false;
    }

    size_t key_len = 0;
    size_t iv_len = 0;
    if (!win_locate_key(unpacked, (size_t)dest_len, key, &key_len, iv, &iv_len))
    {
        s_free(unpacked);
        return false;
    }

    const int8_t cdy = win_cdy_from_lengths(key_len, iv_len);
    if (cdy < 0)
    {
        LOG_ERROR("Windows ZSM 未知密钥长度: key_len=%zu iv_len=%zu", key_len, iv_len);
        s_free(unpacked);
        return false;
    }

    cipher_interface_t* c = win_create_cipher(cdy, key, key_len, iv, iv_len);
    s_free(unpacked);
    if (c == NULL)
    {
        LOG_ERROR("Windows ZSM 创建密码器失败 (cdy %d)", cdy);
        return false;
    }
    LOG_INFO("Windows ZSM cdy 类型: %d", cdy);

    g_prog_status[tl_thread_idx].auth_cfg.type = cdy;
    g_prog_status[tl_thread_idx].auth_cfg.cipher = c;

    /* 存档 key/iv, 供进程被强杀后下次启动补登出时重建密码器 */
    ios_zsm_blob_t* blob = &g_prog_status[tl_thread_idx].auth_cfg.blob;
    blob->key = s_malloc(key_len + 1);
    memcpy(blob->key, key, key_len);
    blob->key[key_len] = 0;
    blob->key_len = key_len;
    if (iv_len)
    {
        blob->iv = s_malloc(iv_len + 1);
        memcpy(blob->iv, iv, iv_len);
        blob->iv[iv_len] = 0;
        blob->iv_len = iv_len;
    }
    snprintf(blob->algo_id, ALGO_ID_LEN, "%s", algo_id_out ? algo_id_out : "");

    LOG_INFO("Windows ZSM 加解密工厂已就绪");
    return true;
}
