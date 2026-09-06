#include "cipher/IosZsm.h"
#include "cipher/CipherInterface.h"
#include "cipher/CipherUtils.h"
#include "utils/Logger.h"
#include "States.h"

#include "LzmaDec.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * iOS PacketTunnel IZsmModLoad (sub_10007131C) layout:
 *
 *   [3-byte hdr][u8 len1][str1][u8 len2][str2=AID]
 *   [5-byte LZMA props][u32le packed: top nibble type==2, low 28 bits unpacked size]
 *   [TEA ciphertext...]
 *
 * TEA key (32 ASCII bytes, two 16-byte halves, 32 decrypt rounds each, 8-byte blocks):
 *   "Rirn53a;feb#UXES5ZrRBTGmYwml:fRt"
 *
 * After LZMA:
 *   buf[0xFA] = IV length
 *   buf[0xFC] = key length
 *   buf[0xFF] = key offset base (key at buf[buf[0xFF]+1])
 *   JS source at buf+0x103
 *
 * JS globals: cdckey, cdciv, cdy(type, mode, key, iv, data)
 * type 常写在 var codex = 0xNN; 再 cdy(codex, ...)
 * type < 16 → oCode (1-9), type >= 16 → nCode (not ported yet)
 */

#define ZSM_TEA_KEY "Rirn53a;feb#UXES5ZrRBTGmYwml:fRt"
#define ZSM_TEA_DELTA 0x61C88647u
#define ZSM_JS_OFFSET 0x103
#define ZSM_MAX_UNPACKED 0x8000000u

typedef struct
{
    char algo_id[ALGO_ID_LEN];
    uint8_t* key;
    size_t key_len;
    uint8_t* iv;
    size_t iv_len;
    char* js;
} ios_zsm_blob_t;

static void* lzma_alloc(ISzAllocPtr p, size_t size)
{
    (void)p;
    return size ? malloc(size) : NULL;
}

static void lzma_free(ISzAllocPtr p, void* address)
{
    (void)p;
    free(address);
}

static const ISzAlloc g_lzma_alloc = { lzma_alloc, lzma_free };

static void zsm_blob_free(ios_zsm_blob_t* blob)
{
    if (!blob) return;
    s_free(blob->key);
    s_free(blob->iv);
    s_free(blob->js);
    memset(blob, 0, sizeof(*blob));
}

static void zsm_tean_decrypt_block(const uint8_t key16[16], uint8_t block[8])
{
    uint32_t k[4];
    uint32_t v0;
    uint32_t v1;
    uint32_t sum;
    int i;

    for (i = 0; i < 4; i++)
    {
        k[i] = bytes_2_uint32_le(key16 + (size_t)i * 4);
    }
    v0 = bytes_2_uint32_le(block);
    v1 = bytes_2_uint32_le(block + 4);
    sum = ZSM_TEA_DELTA * (uint32_t)(-32); /* 0xC6EF3720 */
    do
    {
        v1 -= ((v0 << 4) ^ (v0 >> 5)) + (k[(sum >> 11) & 3] + (v0 ^ sum));
        sum += ZSM_TEA_DELTA;
        v0 -= (k[sum & 3] + (v1 ^ sum)) + ((v1 << 4) ^ (v1 >> 5));
    } while (sum != 0);
    uint32_2_bytes_le(v0, block);
    uint32_2_bytes_le(v1, block + 4);
}

static void zsm_tea_decrypt_buffer(uint8_t* data, size_t length)
{
    static const uint8_t key[] = ZSM_TEA_KEY;
    size_t off;

    for (off = 0; off < length; off += 8)
    {
        zsm_tean_decrypt_block(key, data + off);
        zsm_tean_decrypt_block(key + 16, data + off);
    }
}

static bool zsm_copy_uuid(char* dst, const uint8_t* src, size_t len)
{
    size_t i;
    if (dst == NULL || src == NULL || len != 36)
    {
        return false;
    }
    for (i = 0; i < 36; i++)
    {
        const unsigned char c = src[i];
        const int is_hex = isxdigit(c);
        const int is_dash = (i == 8 || i == 13 || i == 18 || i == 23) && c == '-';
        if (!is_hex && !is_dash)
        {
            return false;
        }
        dst[i] = (char)toupper(c);
    }
    dst[36] = '\0';
    return true;
}

static bool is_js_ident_start(unsigned char c)
{
    return isalpha(c) || c == '_' || c == '$';
}

static bool is_js_ident_cont(unsigned char c)
{
    return isalnum(c) || c == '_' || c == '$';
}

static void skip_js_ws_and_comments(const char** pp)
{
    const char* p = *pp;
    while (*p)
    {
        if (isspace((unsigned char)*p))
        {
            p++;
            continue;
        }
        if (p[0] == '/' && p[1] == '/')
        {
            p += 2;
            while (*p && *p != '\n')
            {
                p++;
            }
            continue;
        }
        if (p[0] == '/' && p[1] == '*')
        {
            p += 2;
            while (*p && !(p[0] == '*' && p[1] == '/'))
            {
                p++;
            }
            if (*p)
            {
                p += 2;
            }
            continue;
        }
        break;
    }
    *pp = p;
}

static bool parse_js_int(const char* s, const char** end, int* out)
{
    unsigned long v;
    char* parsed = NULL;

    if (s == NULL || *s == '\0')
    {
        return false;
    }
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
    {
        v = strtoul(s, &parsed, 16);
    }
    else if (isdigit((unsigned char)*s))
    {
        v = strtoul(s, &parsed, 10);
    }
    else
    {
        return false;
    }
    if (parsed == s || v > 32)
    {
        return false;
    }
    *out = (int)v;
    if (end)
    {
        *end = parsed;
    }
    return true;
}

static int lookup_js_int_var(const char* js, const char* name, size_t name_len)
{
    const char* p = js;
    int last = -1;

    if (js == NULL || name == NULL || name_len == 0)
    {
        return -1;
    }
    while ((p = strstr(p, name)) != NULL)
    {
        const char* q;
        int n;
        const char* end;

        if (p > js && is_js_ident_cont((unsigned char)p[-1]))
        {
            p += name_len;
            continue;
        }
        if (is_js_ident_cont((unsigned char)p[name_len]))
        {
            p += name_len;
            continue;
        }
        q = p + name_len;
        skip_js_ws_and_comments(&q);
        if (*q != '=')
        {
            p += name_len;
            continue;
        }
        q++;
        skip_js_ws_and_comments(&q);
        if (parse_js_int(q, &end, &n) == false)
        {
            p += name_len;
            continue;
        }
        last = n;
        p = end;
    }
    return last;
}

static int parse_cdy_type(const char* js)
{
    const char* p = js;
    int first = -1;
    int from_codex;

    if (p == NULL)
    {
        return -1;
    }

    /*
     * PacketTunnel 下发的模块几乎都是:
     *   var codex = 0x05;
     *   function e(v) { return cdy(codex, 0, cdckey, cdciv, v); }
     * 第一参数是变量, 不是字面量. 同时兼容 cdy(5, ...) / cdy(0x05, ...).
     */
    from_codex = lookup_js_int_var(js, "codex", 5);
    if (from_codex >= 1)
    {
        LOG_INFO("iOS ZSM JS codex = %d (0x%02X)", from_codex, from_codex);
    }

    while ((p = strstr(p, "cdy")) != NULL)
    {
        const char* q = p + 3;
        int n = -1;
        const char* end;

        if (p > js && is_js_ident_cont((unsigned char)p[-1]))
        {
            p += 3;
            continue;
        }
        if (is_js_ident_cont((unsigned char)*q))
        {
            p += 3;
            continue;
        }
        skip_js_ws_and_comments(&q);
        if (*q != '(')
        {
            p += 3;
            continue;
        }
        q++;
        skip_js_ws_and_comments(&q);
        if (parse_js_int(q, &end, &n))
        {
            q = end;
        }
        else if (is_js_ident_start((unsigned char)*q))
        {
            const char* id = q;
            size_t id_len;
            while (is_js_ident_cont((unsigned char)*q))
            {
                q++;
            }
            id_len = (size_t)(q - id);
            if (id_len == 5 && memcmp(id, "codex", 5) == 0 && from_codex >= 0)
            {
                n = from_codex;
            }
            else
            {
                n = lookup_js_int_var(js, id, id_len);
            }
        }
        p += 3;
        if (n < 1 || n > 32)
        {
            continue;
        }
        if (first < 0)
        {
            first = n;
        }
        else if (first != n)
        {
            LOG_WARN("ZSM JS 中存在多个 cdy 类型: %d 和 %d, 使用 %d", first, n, first);
        }
    }
    if (first < 0 && from_codex >= 1 && from_codex <= 32)
    {
        return from_codex;
    }
    return first;
}

static void copy_padded(uint8_t* dst, size_t dst_len, const uint8_t* src, size_t src_len, const char* what)
{
    memset(dst, 0, dst_len);
    if (src == NULL || src_len == 0)
    {
        LOG_WARN("iOS ZSM %s 为空, 按 %zu 字节零填充", what, dst_len);
        return;
    }
    if (src_len != dst_len)
    {
        LOG_WARN("iOS ZSM %s 长度 %zu, 算法需要 %zu, 将截断或补零", what, src_len, dst_len);
    }
    memcpy(dst, src, src_len < dst_len ? src_len : dst_len);
}

static cipher_interface_t* create_ios_ocode_cipher(int type, const uint8_t* key, size_t key_len,
                                                   const uint8_t* iv, size_t iv_len)
{
    uint8_t key_buf[48];
    uint8_t iv_buf[16];
    uint8_t rotated[48];

    switch (type)
    {
    case 1: /* 双层 AES-128-ECB */
        copy_padded(key_buf, 32, key, key_len, "key");
        return create_aes_double_ecb_android_cipher(key_buf);
    case 2: /* 双层 AES-128-CBC */
        copy_padded(key_buf, 32, key, key_len, "key");
        copy_padded(iv_buf, 16, iv, iv_len, "iv");
        return create_aes_double_cbc_android_cipher(key_buf, iv_buf);
    case 3: /* 六轮 DES-ECB: iOS 顺序是 K4,K5,K6,K1,K2,K3, 旋转后复用 Android 实现 */
        copy_padded(key_buf, 48, key, key_len, "key");
        memcpy(rotated, key_buf + 24, 24);
        memcpy(rotated + 24, key_buf, 24);
        return create_des_ecb_six_android_cipher(rotated);
    case 4: /* 双层 3DES-CBC, 第一轮 key[24:48], 第二轮 key[0:24] */
        copy_padded(key_buf, 48, key, key_len, "key");
        copy_padded(iv_buf, 8, iv, iv_len, "iv");
        return create_desede_double_cbc_android_cipher(key_buf, iv_buf);
    case 5: /* 三层改 TEA-ECB */
        copy_padded(key_buf, 48, key, key_len, "key");
        return create_tea_triple_ecb_android_cipher(key_buf);
    case 6: /* 三层改 TEA-CBC */
        copy_padded(key_buf, 48, key, key_len, "key");
        copy_padded(iv_buf, 8, iv, iv_len, "iv");
        return create_tea_triple_cbc_android_cipher(key_buf, iv_buf);
    case 7: /* SM4-ECB */
        copy_padded(key_buf, 16, key, key_len, "key");
        return create_sm4_variant_ecb_android_cipher(key_buf);
    case 8: /* SM4-CBC */
        copy_padded(key_buf, 16, key, key_len, "key");
        copy_padded(iv_buf, 16, iv, iv_len, "iv");
        return create_sm4_variant_cbc_android_cipher(key_buf, iv_buf);
    case 9: /* SNOW3G 变体 */
        copy_padded(key_buf, 16, key, key_len, "key");
        copy_padded(iv_buf, 16, iv, iv_len, "iv");
        return create_snow3g_variant_android_cipher(key_buf, iv_buf);
    default:
        return NULL;
    }
}


static void log_zsm_prefix(const uint8_t* data, size_t length)
{
    char hex[97];
    size_t n = length < 32 ? length : 32;
    size_t i;
    for (i = 0; i < n; i++)
    {
        sprintf(hex + i * 3, "%02X ", data[i]);
    }
    if (n)
    {
        hex[n * 3 - 1] = '\0';
    }
    else
    {
        hex[0] = '\0';
    }
    LOG_INFO("ZSM 前 %zu 字节: %s", n, hex);
}

bool looks_like_ios_zsm(const uint8_t* data, size_t length)
{
    size_t offset;
    uint8_t len1;
    uint8_t len2;
    uint32_t packed;
    uint32_t type_nibble;
    uint32_t unpacked_size;

    if (data == NULL || length < 15)
    {
        return false;
    }
    offset = 3;
    len1 = data[offset++];
    if (offset + len1 + 1 > length)
    {
        return false;
    }
    offset += len1;
    len2 = data[offset++];
    if (offset + len2 + 9 > length)
    {
        return false;
    }
    offset += len2;
    packed = bytes_2_uint32_le(data + offset + 5);
    type_nibble = packed >> 28;
    unpacked_size = packed & 0x0FFFFFFFu;
    return type_nibble == 2 && unpacked_size >= 1 && unpacked_size <= ZSM_MAX_UNPACKED;
}

static bool unwrap_ios_zsm(const uint8_t* data, size_t length, ios_zsm_blob_t* out)
{
    size_t offset;
    uint8_t len1;
    uint8_t len2;
    const uint8_t* str2;
    const uint8_t* props;
    uint32_t packed;
    uint32_t unpacked_size;
    uint32_t type_nibble;
    size_t remain;
    size_t cipher_len;
    uint8_t* cipher = NULL;
    uint8_t pad;
    SizeT src_len;
    SizeT dest_len;
    uint8_t* unpacked = NULL;
    ELzmaStatus status;
    SRes lzma_ret;
    uint8_t key_off;
    uint8_t key_len;
    uint8_t iv_len;
    const uint8_t* key_ptr;
    const uint8_t* iv_ptr;
    const char* js;

    memset(out, 0, sizeof(*out));
    if (data == NULL || length < 15)
    {
        LOG_ERROR("iOS ZSM 太短: %zu", length);
        if (data && length)
        {
            log_zsm_prefix(data, length);
        }
        return false;
    }
    log_zsm_prefix(data, length);

    offset = 3;
    len1 = data[offset++];
    if (offset + len1 + 1 > length)
    {
        LOG_ERROR("iOS ZSM str1 越界");
        return false;
    }
    offset += len1;
    len2 = data[offset++];
    if (offset + len2 > length)
    {
        LOG_ERROR("iOS ZSM str2 越界");
        return false;
    }
    str2 = data + offset;
    if (zsm_copy_uuid(out->algo_id, str2, len2))
    {
        LOG_INFO("iOS ZSM Algo-ID: %s", out->algo_id);
    }
    else
    {
        LOG_WARN("iOS ZSM str2 不是 UUID (len=%u), 仍继续解包", len2);
        snprintf(out->algo_id, ALGO_ID_LEN, "00000000-0000-0000-0000-000000000000");
    }
    offset += len2;
    remain = length - offset;
    if (remain <= 9)
    {
        LOG_ERROR("iOS ZSM 剩余长度不足: %zu", remain);
        return false;
    }

    props = data + offset;
    packed = bytes_2_uint32_le(props + 5);
    type_nibble = packed >> 28;
    unpacked_size = packed & 0x0FFFFFFFu;
    if (type_nibble != 2 || unpacked_size == 0 || unpacked_size > ZSM_MAX_UNPACKED)
    {
        LOG_ERROR("iOS ZSM packed 头非法: type=%u size=%u remain=%zu props=%02X %02X %02X %02X %02X",
                  type_nibble, unpacked_size, remain,
                  props[0], props[1], props[2], props[3], props[4]);
        return false;
    }

    cipher_len = remain - 9;
    cipher = s_calloc(1, cipher_len + 8);
    memcpy(cipher, props + 9, cipher_len);
    zsm_tea_decrypt_buffer(cipher, cipher_len);

    pad = cipher_len ? cipher[cipher_len - 1] : 0;
    if (pad > cipher_len)
    {
        LOG_ERROR("iOS ZSM TEA 填充长度非法: %u / %zu", pad, cipher_len);
        s_free(cipher);
        return false;
    }
    src_len = (SizeT)(cipher_len - pad);
    dest_len = (SizeT)unpacked_size;
    unpacked = s_calloc(1, (size_t)unpacked_size + 1);
    lzma_ret = LzmaDecode(unpacked, &dest_len, cipher, &src_len, props, 5,
                          LZMA_FINISH_ANY, &status, &g_lzma_alloc);
    s_free(cipher);
    cipher = NULL;
    if (lzma_ret != SZ_OK)
    {
        LOG_ERROR("iOS ZSM LZMA 解压失败: %d", lzma_ret);
        s_free(unpacked);
        return false;
    }

    key_off = unpacked[0xFF];
    key_len = unpacked[0xFC];
    iv_len = unpacked[0xFA];
    if ((unsigned)key_off + (unsigned)key_len + (unsigned)iv_len >= 0xFA || dest_len <= ZSM_JS_OFFSET)
    {
        LOG_ERROR("iOS ZSM 明文头非法: key_off=%u key_len=%u iv_len=%u unpacked=%u",
                  key_off, key_len, iv_len, (unsigned)dest_len);
        s_free(unpacked);
        return false;
    }

    key_ptr = unpacked + key_off + 1;
    iv_ptr = key_ptr + key_len;
    js = (const char*)(unpacked + ZSM_JS_OFFSET);

    out->key_len = key_len;
    out->key = s_malloc(key_len + 1);
    memcpy(out->key, key_ptr, key_len);
    out->key[key_len] = 0;

    if (iv_len == 0 || (iv_len == 4 && memcmp(iv_ptr, "noiv", 4) == 0))
    {
        out->iv = NULL;
        out->iv_len = 0;
    }
    else
    {
        out->iv_len = iv_len;
        out->iv = s_malloc(iv_len + 1);
        memcpy(out->iv, iv_ptr, iv_len);
        out->iv[iv_len] = 0;
    }

    out->js = s_malloc(strlen(js) + 1);
    memcpy(out->js, js, strlen(js) + 1);
    s_free(unpacked);

    LOG_INFO("iOS ZSM 解包成功: key_len=%zu iv_len=%zu js_len=%zu",
             out->key_len, out->iv_len, strlen(out->js));
    LOG_DEBUG("iOS ZSM JS 开头: %.240s", out->js);
    if (strstr(out->js, "pxs") || strstr(out->js, "txs"))
    {
        LOG_WARN("iOS ZSM JS 调用了 pxs/txs, 当前只执行 cdy 加解密, 若认证失败需要把 JS 贴出来");
    }
    return true;
}

bool init_ios_cipher_from_zsm(const uint8_t* data, size_t length, char* algo_id_out)
{
    ios_zsm_blob_t blob;
    int type;
    cipher_interface_t* cipher;

    if (!unwrap_ios_zsm(data, length, &blob))
    {
        return false;
    }
    if (algo_id_out)
    {
        snprintf(algo_id_out, ALGO_ID_LEN, "%s", blob.algo_id);
    }

    type = parse_cdy_type(blob.js);
    if (type < 0)
    {
        LOG_ERROR("iOS ZSM JS 中没有找到 cdy 类型 (literal 或 var codex = 0xNN), 无法选择算法");
        LOG_ERROR("JS: %.400s", blob.js);
        zsm_blob_free(&blob);
        return false;
    }
    LOG_INFO("iOS ZSM cdy 类型: %d", type);
    if (type >= 16)
    {
        LOG_ERROR("iOS ZSM 使用 nCode 类型 %d, 目前只移植了 oCode 1-9", type);
        zsm_blob_free(&blob);
        return false;
    }

    cipher = create_ios_ocode_cipher(type, blob.key, blob.key_len, blob.iv, blob.iv_len);
    zsm_blob_free(&blob);
    if (cipher == NULL)
    {
        LOG_ERROR("iOS ZSM 无法创建类型 %d 的加解密工厂", type);
        return false;
    }

    g_prog_status[tl_thread_idx].auth_cfg.cipher = cipher;
    LOG_DEBUG("iOS ZSM 加解密工厂已就绪");
    return true;
}
