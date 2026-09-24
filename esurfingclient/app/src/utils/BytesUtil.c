#include "utils/PlatformUtils.h"

#include "states/States.h"

#include "utils/PlatformInternal.h"

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>

static const char b64_enc[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

char* bytes2base64(const uint8_t* in, const size_t len)
{
    if (in == NULL && len > 0) return NULL;

    const size_t olen = 4 * ((len + 2) / 3);
    char* out = malloc(olen + 1);
    if (!out) return NULL;

    size_t i = 0, j = 0;
    while (i < len)
    {
        const uint32_t a = i < len ? in[i++] : 0;
        const uint32_t b = i < len ? in[i++] : 0;
        const uint32_t c = i < len ? in[i++] : 0;
        const uint32_t t = (a << 16) | (b << 8) | c;

        out[j++] = b64_enc[(t >> 18) & 0x3F];
        out[j++] = b64_enc[(t >> 12) & 0x3F];
        out[j++] = b64_enc[(t >> 6)  & 0x3F];
        out[j++] = b64_enc[t & 0x3F];
    }

    const size_t mod = len % 3;
    if (mod == 1) { out[olen - 1] = '='; out[olen - 2] = '='; }
    else if (mod == 2) { out[olen - 1] = '='; }

    out[olen] = '\0';
    return out;
}

uint8_t* base642bytes(const char* in, size_t* out_len)
{
    if (in == NULL || out_len == NULL) return NULL;

    const size_t in_len = strlen(in);
    if (in_len == 0)
    {
        uint8_t* p = malloc(1);
        if (p) *out_len = 0;
        return p;
    }
    if (in_len % 4 != 0) return NULL;

    static int dec[256];
    static int inited = 0;
    if (!inited)
    {
        memset(dec, -1, sizeof(dec));
        for (int i = 0; i < 64; i++) dec[(uint8_t)b64_enc[i]] = i;
        inited = 1;
    }

    const size_t max_out = in_len / 4 * 3;
    uint8_t* out = malloc(max_out);
    if (!out) return NULL;

    size_t j = 0;
    for (size_t i = 0; i < in_len; i += 4)
    {
        const int v0 = dec[(uint8_t)in[i]];
        const int v1 = dec[(uint8_t)in[i + 1]];
        const int v2 = in[i + 2] == '=' ? -2 : dec[(uint8_t)in[i + 2]];
        const int v3 = in[i + 3] == '=' ? -2 : dec[(uint8_t)in[i + 3]];

        if (v0 < 0 || v1 < 0 || v2 == -1 || v3 == -1) { free(out); return NULL; }

        const uint32_t t = (v0 << 18) | (v1 << 12) |
                     ((v2 < 0 ? 0 : v2) << 6) |
                      (v3 < 0 ? 0 : v3);

        out[j++] = (t >> 16) & 0xFF;
        if (v2 >= 0) out[j++] = (t >> 8) & 0xFF;
        if (v3 >= 0) out[j++] = t & 0xFF;
    }

    *out_len = j;
    return out;
}

bytes_t str2bytes(const char* str)
{
    bytes_t ba = {0};
    if (!str) return ba;
    ba.len = strlen(str);
    ba.data = (uint8_t*)malloc(ba.len);
    if (ba.data) memcpy(ba.data, str, ba.len);
    return ba;
}

uint64_t str2uint64(const char* str)
{
    if (!str) return 0;
    while (isspace(*str)) str++;
    if (*str == '\0') return 0;
    char* end_ptr;
    errno = 0;
    const uint64_t value = strtoll(str, &end_ptr, 10);
    if (errno == ERANGE) return 0;
    if (end_ptr == str) return 0;
    while (isspace(*end_ptr)) end_ptr++;
    if (*end_ptr != '\0') return 0;
    return value;
}

char* uint642str(const uint64_t num)
{
    char* result = malloc(22);
    if (!result) return NULL;
    snprintf(result, 22, "%" PRIu64, num);
    return result;
}

void get_rand_bytes(uint8_t* buf, const size_t len)
{
#ifdef _WIN32
    HCRYPTPROV h_crypt_prov;
    if (!CryptAcquireContext(&h_crypt_prov, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) return;
    CryptGenRandom(h_crypt_prov, len, buf);
    CryptReleaseContext(h_crypt_prov, 0);
#else
    const int fd = open("/dev/urandom", O_RDONLY);
    if (fd == -1) return;
    read(fd, buf, len);
    close(fd);
#endif
}
