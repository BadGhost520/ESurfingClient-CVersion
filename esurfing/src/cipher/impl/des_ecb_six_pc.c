#include "../../headFiles/cipher/CipherInterface.h"
#include "../../headFiles/cipher/CipherUtils.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static const int IP[64] = {
    58,50,42,34,26,18,10,2,
    60,52,44,36,28,20,12,4,
    62,54,46,38,30,22,14,6,
    64,56,48,40,32,24,16,8,
    57,49,41,33,25,17,9,1,
    59,51,43,35,27,19,11,3,
    61,53,45,37,29,21,13,5,
    63,55,47,39,31,23,15,7
};
static const int FP[64] = {
    40,8,48,16,56,24,64,32,
    39,7,47,15,55,23,63,31,
    38,6,46,14,54,22,62,30,
    37,5,45,13,53,21,61,29,
    36,4,44,12,52,20,60,28,
    35,3,43,11,51,19,59,27,
    34,2,42,10,50,18,58,26,
    33,1,41,9,49,17,57,25
};
static const int E_EXP[48] = {
    32, 1, 2, 3, 4, 5,
     4, 5, 6, 7, 8, 9,
     8, 9,10,11,12,13,
    12,13,14,15,16,17,
    16,17,18,19,20,21,
    20,21,22,23,24,25,
    24,25,26,27,28,29,
    28,29,30,31,32, 1
};
static const int P_PERM[32] = {
    16,7,20,21,29,12,28,17,
     1,15,23,26,5,18,31,10,
     2,8,24,14,32,27,3,9,
    19,13,30,6,22,11,4,25
};
static const int PC1[56] = {
    57,49,41,33,25,17,9,
     1,58,50,42,34,26,18,
    10,2,59,51,43,35,27,
    19,11,3,60,52,44,36,
    63,55,47,39,31,23,15,
     7,62,54,46,38,30,22,
    14,6,61,53,45,37,29,
    21,13,5,28,20,12,4
};
static const int PC2[48] = {
    14,17,11,24,1,5,
     3,28,15,6,21,10,
    23,19,12,4,26,8,
    16,7,27,20,13,2,
    41,52,31,37,47,55,
    30,40,51,45,33,48,
    44,49,39,56,34,53,
    46,42,50,36,29,32
};
static const int SHIFTS[16] = {1,1,2,2,2,2,2,2,1,2,2,2,2,2,2,1};
static const uint8_t SBOX[8][64] = {
  {14,4,13,1,2,15,11,8,3,10,6,12,5,9,0,7,
   0,15,7,4,14,2,13,1,10,6,12,11,9,5,3,8,
   4,1,14,8,13,6,2,11,15,12,9,7,3,10,5,0,
   15,12,8,2,4,9,1,7,5,11,3,14,10,0,6,13},
  {15,1,8,14,6,11,3,4,9,7,2,13,12,0,5,10,
   3,13,4,7,15,2,8,14,12,0,1,10,6,9,11,5,
   0,14,7,11,10,4,13,1,5,8,12,6,9,3,2,15,
   13,8,10,1,3,15,4,2,11,6,7,12,0,5,14,9},
  {10,0,9,14,6,3,15,5,1,13,12,7,11,4,2,8,
   13,7,0,9,3,4,6,10,2,8,5,14,12,11,15,1,
   13,6,4,9,8,15,3,0,11,1,2,12,5,10,14,7,
   1,10,13,0,6,9,8,7,4,15,14,3,11,5,2,12},
  {7,13,14,3,0,6,9,10,1,2,8,5,11,12,4,15,
   13,8,11,5,6,15,0,3,4,7,2,12,1,10,14,9,
   10,6,9,0,12,11,7,13,15,1,3,14,5,2,8,4,
   3,15,0,6,10,1,13,8,9,4,5,11,12,7,2,14},
  {2,12,4,1,7,10,11,6,8,5,3,15,13,0,14,9,
   14,11,2,12,4,7,13,1,5,0,15,10,3,9,8,6,
   4,2,1,11,10,13,7,8,15,9,12,5,6,3,0,14,
   11,8,12,7,1,14,2,13,6,15,0,9,10,4,5,3},
  {12,1,10,15,9,2,6,8,0,13,3,4,14,7,5,11,
   10,15,4,2,7,12,9,5,6,1,13,14,0,11,3,8,
   9,14,15,5,2,8,12,3,7,0,4,10,1,13,11,6,
   4,3,2,12,9,5,15,10,11,14,1,7,6,0,8,13},
  {4,11,2,14,15,0,8,13,3,12,9,7,5,10,6,1,
   13,0,11,7,4,9,1,10,14,3,5,12,2,15,8,6,
   1,4,11,13,12,3,7,14,10,15,6,8,0,5,9,2,
   6,11,13,8,1,4,10,7,9,5,0,15,14,2,3,12},
  {13,2,8,4,6,15,11,1,10,9,3,14,5,0,12,7,
   1,15,13,8,10,3,7,4,12,5,6,11,0,14,9,2,
   7,11,4,1,9,12,14,2,0,6,10,13,15,3,5,8,
   2,1,14,7,4,10,8,13,15,12,9,0,3,5,6,11}
};

typedef struct
{
    uint64_t subkeys[16];
} des_key_t;

static uint64_t left_rotate28(uint64_t v, const int s)
{
    v &= 0x0FFFFFFF;
    return (v << s | v >> (28 - s)) & 0x0FFFFFFF;
}

static uint64_t permute64(const uint64_t in, const int* table)
{
    uint64_t out = 0;
    for (int i = 0; i < 64; i++)
    {
        const int src = table[i] - 1;
        out <<= 1;
        out |= in >> (64 - 1 - src) & 1ULL;
    }
    return out;
}

static uint32_t feistel(const uint32_t r, const uint64_t subkey)
{
    uint64_t e = 0;
    for (int i = 0; i < 48; i++)
    {
        const int src = E_EXP[i] - 1;
        e <<= 1;
        e |= (uint64_t)(r >> (32 - 1 - src) & 1U);
    }
    e ^= subkey;
    uint32_t s_out = 0;
    for (int i = 0; i < 8; i++)
    {
        const uint8_t six = e >> (48 - 6*(i+1)) & 0x3F;
        const int row = (six & 0x20) >> 4 | six & 0x01;
        const int col = six >> 1 & 0x0F;
        const uint8_t val = SBOX[i][row*16 + col];
        s_out = s_out << 4 | val;
    }
    uint32_t p = 0;
    for (int i = 0; i < 32; i++)
    {
        const int src = P_PERM[i] - 1;
        p <<= 1;
        p |= (s_out >> (32 - 1 - src)) & 1U;
    }
    return p;
}

static des_key_t des_schedule(const uint8_t key_bytes[8])
{
    uint64_t key64 = 0;
    for (int i = 0; i < 8; i++) key64 = key64 << 8 | key_bytes[i];
    uint64_t kp = 0;
    for (int i = 0; i < 56; i++)
    {
        const int src = PC1[i] - 1;
        kp <<= 1;
        kp |= key64 >> (64 - 1 - src) & 1ULL;
    }
    uint32_t c = (uint32_t)(kp >> 28 & 0x0FFFFFFF);
    uint32_t d = (uint32_t)(kp & 0x0FFFFFFF);
    des_key_t sched;
    for (int round = 0; round < 16; round++)
    {
        c = (uint32_t)left_rotate28(c, SHIFTS[round]);
        d = (uint32_t)left_rotate28(d, SHIFTS[round]);
        const uint64_t cd = (uint64_t)c << 28 | d;
        uint64_t sk = 0;
        for (int i = 0; i < 48; i++) {
            const int src = PC2[i] - 1;
            sk <<= 1;
            sk |= cd >> (56 - 1 - src) & 1ULL;
        }
        sched.subkeys[round] = sk;
    }
    return sched;
}

static uint64_t des_block_core(const uint64_t block, const des_key_t* sched, const int decrypt)
{
    uint64_t ip = permute64(block, IP);
    uint32_t l = (uint32_t)(ip >> 32);
    uint32_t r = (uint32_t)ip;
    for (int round = 0; round < 16; round++)
    {
        const uint64_t sk = decrypt ? sched->subkeys[15 - round] : sched->subkeys[round];
        const uint32_t f = feistel(r, sk);
        const uint32_t new_l = r;
        const uint32_t new_r = l ^ f;
        l = new_l; r = new_r;
    }
    const uint64_t preout = ((uint64_t)r << 32) | l;
    const uint64_t fp = permute64(preout, FP);
    return fp;
}

static uint64_t read_be64(const uint8_t* in)
{
    uint64_t x = 0;
    for (int i = 0; i < 8; i++) x = x << 8 | in[i];
    return x;
}
static void write_be64(uint8_t* out, const uint64_t x)
{
    for (int i = 0; i < 8; i++) out[i] = (uint8_t)(x >> (56 - 8*i));
}

typedef struct {
    des_key_t ks[6];
} des_ecb_six_pc_ctx_t;

static char* des_ecb_six_pc_encrypt(cipher_interface_t* self, const char* text)
{
    if (!self || !text) return NULL;
    const des_ecb_six_pc_ctx_t* ctx = self->private_data;
    const size_t len = safeStrLen(text);
    size_t padded_len = 0;
    uint8_t* padded = padToMultiple((const uint8_t*)text, len, 8, &padded_len);
    if (!padded) return NULL;
    uint8_t* out = safeMalloc(padded_len);
    for (size_t i = 0; i < padded_len; i += 8)
    {
        uint64_t b = read_be64(padded + i);
        b = des_block_core(b, &ctx->ks[3], 0);
        b = des_block_core(b, &ctx->ks[4], 1);
        b = des_block_core(b, &ctx->ks[5], 0);
        b = des_block_core(b, &ctx->ks[0], 0);
        b = des_block_core(b, &ctx->ks[1], 1);
        b = des_block_core(b, &ctx->ks[2], 0);
        write_be64(out + i, b);
    }
    char* hex = bytesToHexUpper(out, padded_len);
    safeFree(padded);
    safeFree(out);
    return hex;
}

static char* des_ecb_six_pc_decrypt(cipher_interface_t* self, const char* hex)
{
    if (!self || !hex) return NULL;
    const des_ecb_six_pc_ctx_t* ctx = self->private_data;
    size_t in_len = 0;
    uint8_t* in = hexToBytes(hex, &in_len);
    if (!in || (in_len % 8) != 0)
    {
        safeFree(in); return NULL;
    }
    uint8_t* out = safeMalloc(in_len);
    for (size_t i = 0; i < in_len; i += 8)
    {
        uint64_t b = read_be64(in + i);
        b = des_block_core(b, &ctx->ks[2], 1);
        b = des_block_core(b, &ctx->ks[1], 0);
        b = des_block_core(b, &ctx->ks[0], 1);
        b = des_block_core(b, &ctx->ks[5], 1);
        b = des_block_core(b, &ctx->ks[4], 0);
        b = des_block_core(b, &ctx->ks[3], 1);
        write_be64(out + i, b);
    }
    size_t plain_len = in_len;
    while (plain_len > 0 && out[plain_len - 1] == 0x00) plain_len--;
    char* text = safeMalloc(plain_len + 1);
    memcpy(text, out, plain_len);
    text[plain_len] = '\0';
    safeFree(in);
    safeFree(out);
    return text;
}

static void des_ecb_six_pc_destroy(cipher_interface_t* self)
{
    if (!self) return;
    if (self->private_data) safeFree(self->private_data);
    safeFree(self);
}

cipher_interface_t* create_des_ecb_six_pc_cipher(
    const uint8_t* key0,
    const uint8_t* key1,
    const uint8_t* key2,
    const uint8_t* key3,
    const uint8_t* key4,
    const uint8_t* key5
)
{
    if (!key0 || !key1 || !key2 || !key3 || !key4 || !key5) return NULL;
    cipher_interface_t* ci = safeCalloc(1, sizeof(cipher_interface_t));
    des_ecb_six_pc_ctx_t* ctx = safeCalloc(1, sizeof(des_ecb_six_pc_ctx_t));
    ctx->ks[0] = des_schedule(key0);
    ctx->ks[1] = des_schedule(key1);
    ctx->ks[2] = des_schedule(key2);
    ctx->ks[3] = des_schedule(key3);
    ctx->ks[4] = des_schedule(key4);
    ctx->ks[5] = des_schedule(key5);
    ci->encrypt = des_ecb_six_pc_encrypt;
    ci->decrypt = des_ecb_six_pc_decrypt;
    ci->destroy = des_ecb_six_pc_destroy;
    ci->private_data = ctx;
    return ci;
}