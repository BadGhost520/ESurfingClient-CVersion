#include "cipher/CipherInterface.h"
#include "cipher/CipherUtils.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------
 * Algo Id: AD8BB5B0-0E72-4198-A362-96D52C1B7ED1 (Six-layer DES-ECB, Android)
 * Six DES passes on each 8-byte block: E(K1) D(K2) E(K3) E(K4) D(K5) E(K6)
 * with K1..K6 = key[0..7], key[8..15], ..., key[40..47], ECB mode.
 * Enc core sub_2DCC / dec core sub_2F24 (verified via unicorn).
 * ------------------------------------------------------------------ */

#define DES_SIX_KEY_SIZE 48
#define DES_SIX_BLOCK_SIZE 8

typedef struct {
    uint8_t key[DES_SIX_KEY_SIZE];
} des_ecb_six_android_data_t;

static const int DES_SIX_IP[64] = {58,50,42,34,26,18,10,2,60,52,44,36,28,20,12,4,62,54,46,38,30,22,14,6,64,56,48,40,32,24,16,8,57,49,41,33,25,17,9,1,59,51,43,35,27,19,11,3,61,53,45,37,29,21,13,5,63,55,47,39,31,23,15,7};
static const int DES_SIX_FP[64] = {40,8,48,16,56,24,64,32,39,7,47,15,55,23,63,31,38,6,46,14,54,22,62,30,37,5,45,13,53,21,61,29,36,4,44,12,52,20,60,28,35,3,43,11,51,19,59,27,34,2,42,10,50,18,58,26,33,1,41,9,49,17,57,25};
static const int DES_SIX_PC1[56] = {57,49,41,33,25,17,9,1,58,50,42,34,26,18,10,2,59,51,43,35,27,19,11,3,60,52,44,36,63,55,47,39,31,23,15,7,62,54,46,38,30,22,14,6,61,53,45,37,29,21,13,5,28,20,12,4};
static const int DES_SIX_E_SELECT[48] = {32,1,2,3,4,5,4,5,6,7,8,9,8,9,10,11,12,13,12,13,14,15,16,17,16,17,18,19,20,21,20,21,22,23,24,25,24,25,26,27,28,29,28,29,30,31,32,1};
static const int DES_SIX_PC2[48] = {14,17,11,24,1,5,3,28,15,6,21,10,23,19,12,4,26,8,16,7,27,20,13,2,41,52,31,37,47,55,30,40,51,45,33,48,44,49,39,56,34,53,46,42,50,36,29,32};
static const int DES_SIX_P[32] = {16,7,20,21,29,12,28,17,1,15,23,26,5,18,31,10,2,8,24,14,32,27,3,9,19,13,30,6,22,11,4,25};
static const uint8_t DES_SIX_SBOX[8*64] = {
    14,4,13,1,2,15,11,8,3,10,6,12,5,9,0,7,0,15,7,4,14,2,13,1,10,6,12,11,9,5,3,8,4,1,14,8,13,6,2,11,15,12,9,7,3,10,5,0,15,12,8,2,4,9,1,7,5,11,3,14,10,0,6,13,
    15,1,8,14,6,11,3,4,9,7,2,13,12,0,5,10,3,13,4,7,15,2,8,14,12,0,1,10,6,9,11,5,0,14,7,11,10,4,13,1,5,8,12,6,9,3,2,15,13,8,10,1,3,15,4,2,11,6,7,12,0,5,14,9,
    10,0,9,14,6,3,15,5,1,13,12,7,11,4,2,8,13,7,0,9,3,4,6,10,2,8,5,14,12,11,15,1,13,6,4,9,8,15,3,0,11,1,2,12,5,10,14,7,1,10,13,0,6,9,8,7,4,15,14,3,11,5,2,12,
    7,13,14,3,0,6,9,10,1,2,8,5,11,12,4,15,13,8,11,5,6,15,0,3,4,7,2,12,1,10,14,9,10,6,9,0,12,11,7,13,15,1,3,14,5,2,8,4,3,15,0,6,10,1,13,8,9,4,5,11,12,7,2,14,
    2,12,4,1,7,10,11,6,8,5,3,15,13,0,14,9,14,11,2,12,4,7,13,1,5,0,15,10,3,9,8,6,4,2,1,11,10,13,7,8,15,9,12,5,6,3,0,14,11,8,12,7,1,14,2,13,6,15,0,9,10,4,5,3,
    12,1,10,15,9,2,6,8,0,13,3,4,14,7,5,11,10,15,4,2,7,12,9,5,6,1,13,14,0,11,3,8,9,14,15,5,2,8,12,3,7,0,4,10,1,13,11,6,4,3,2,12,9,5,15,10,11,14,1,7,6,0,8,13,
    4,11,2,14,15,0,8,13,3,12,9,7,5,10,6,1,13,0,11,7,4,9,1,10,14,3,5,12,2,15,8,6,1,4,11,13,12,3,7,14,10,15,6,8,0,5,9,2,6,11,13,8,1,4,10,7,9,5,0,15,14,2,3,12,
    13,2,8,4,6,15,11,1,10,9,3,14,5,0,12,7,1,15,13,8,10,3,7,4,12,5,6,11,0,14,9,2,7,11,4,1,9,12,14,2,0,6,10,13,15,3,5,8,2,1,14,7,4,10,8,13,15,12,9,0,3,5,6,11};
static const int DES_SIX_SHIFTS[16] = {1,1,2,2,2,2,2,2,1,2,2,2,2,2,2,1};

static void des_six_bytes_to_bits(const uint8_t *in, uint8_t *out_bits)
{
    for (int b = 0; b < 8; ++b)
        for (int i = 7; i >= 0; --i)
            *out_bits++ = in[b] >> i & 1u;
}
static void des_six_bits_to_bytes(const uint8_t *in_bits, uint8_t *out)
{
    for (int b = 0; b < 8; ++b)
    {
        uint8_t v = 0;
        for (int i = 0; i < 8; ++i) v = v << 1 | (in_bits[b*8 + i] & 1u);
        out[b] = v;
    }
}
static void des_six_apply_perm(uint8_t *dst_bits, const uint8_t *src_bits,
                               const int *table, const int count)
{
    for (int i = 0; i < count; ++i) dst_bits[i] = src_bits[table[i] - 1] & 1u;
}
static void des_six_rotate_cd(uint8_t *cd56, const int shift)
{
    uint8_t C[28], D[28];
    memcpy(C, cd56, 28); memcpy(D, cd56 + 28, 28);
    for (int i = 0; i < 28; ++i)
    {
        cd56[i] = C[(i + shift) % 28];
        cd56[28 + i] = D[(i + shift) % 28];
    }
}
static void des_six_round_f(const uint8_t R[32], const uint8_t subkey48[48], uint8_t out32[32])
{
    uint8_t expanded[48];
    des_six_apply_perm(expanded, R, DES_SIX_E_SELECT, 48);
    for (int i = 0; i < 48; ++i) expanded[i] ^= subkey48[i];
    uint8_t s_out[32];
    const uint8_t *p = expanded;
    for (int box = 0; box < 8; ++box)
    {
        const int idx = (box << 6) + p[4] + 2 * (p[3] + 2 * (p[2] + 2 * (p[1] + 2 * (p[5] + 2 * p[0]))));
        const uint8_t val = DES_SIX_SBOX[idx] & 0x0F;
        s_out[box*4 + 0] = (val >> 3) & 1;
        s_out[box*4 + 1] = (val >> 2) & 1;
        s_out[box*4 + 2] = (val >> 1) & 1;
        s_out[box*4 + 3] = (val >> 0) & 1;
        p += 6;
    }
    des_six_apply_perm(out32, s_out, DES_SIX_P, 32);
}
static void des_six_key_schedule(const uint8_t key8[8], uint8_t subkeys[16][48])
{
    uint8_t key_bits[64], cd[56], tmp48[48];
    des_six_bytes_to_bits(key8, key_bits);
    des_six_apply_perm(cd, key_bits, DES_SIX_PC1, 56);
    for (int round = 0; round < 16; ++round)
    {
        des_six_rotate_cd(cd, DES_SIX_SHIFTS[round]);
        des_six_apply_perm(tmp48, cd, DES_SIX_PC2, 48);
        memcpy(subkeys[round], tmp48, 48);
    }
}
static void des_six_block_process(const uint8_t in[8], uint8_t out[8],
                                  const uint8_t subkeys[16][48], const int encrypt)
{
    uint8_t in_bits[64], ip[64];
    des_six_bytes_to_bits(in, in_bits);
    des_six_apply_perm(ip, in_bits, DES_SIX_IP, 64);
    uint8_t L[32], R[32];
    memcpy(L, ip, 32); memcpy(R, ip + 32, 32);
    for (int round = 0; round < 16; ++round)
    {
        uint8_t f[32];
        const uint8_t *sk = encrypt ? subkeys[round] : subkeys[15 - round];
        des_six_round_f(R, sk, f);
        uint8_t newR[32];
        for (int i = 0; i < 32; ++i) newR[i] = L[i] ^ f[i];
        memcpy(L, R, 32);
        memcpy(R, newR, 32);
    }
    uint8_t preout[64], out_bits[64];
    memcpy(preout, R, 32);
    memcpy(preout + 32, L, 32);
    des_six_apply_perm(out_bits, preout, DES_SIX_FP, 64);
    des_six_bits_to_bytes(out_bits, out);
}
static void des_six_encrypt_block(const uint8_t in[8], uint8_t out[8], const uint8_t key[8])
{
    uint8_t subkeys[16][48];
    des_six_key_schedule(key, subkeys);
    des_six_block_process(in, out, subkeys, 1);
}
static void des_six_decrypt_block(const uint8_t in[8], uint8_t out[8], const uint8_t key[8])
{
    uint8_t subkeys[16][48];
    des_six_key_schedule(key, subkeys);
    des_six_block_process(in, out, subkeys, 0);
}

/* sub_2DCC: six-layer DES-ECB encrypt (E K4 D K5 E K6 E K1 D K2 E K3) */
static uint8_t* des_six_encrypt_raw(const uint8_t* key, const uint8_t* data,
                                    const size_t data_len, size_t* output_len)
{
    const int total = (int)((data_len + 7) & ~7);
    uint8_t *out = s_calloc(1, total ? (size_t)total : DES_SIX_BLOCK_SIZE);
    if (!out) return NULL;
    memcpy(out, data, data_len);
    for (int off = 0; off < total; off += 8)
    {
        uint8_t t[8], t2[8], t3[8], t4[8], t5[8], t6[8];
        des_six_encrypt_block(out + off, t, key + 24);
        des_six_decrypt_block(t, t2, key + 32);
        des_six_encrypt_block(t2, t3, key + 40);
        des_six_encrypt_block(t3, t4, key + 0);
        des_six_decrypt_block(t4, t5, key + 8);
        des_six_encrypt_block(t5, t6, key + 16);
        memcpy(out + off, t6, 8);
    }
    *output_len = (size_t)total;
    return out;
}

/* sub_2F24: six-layer DES-ECB decrypt (D K3 E K2 D K1 D K6 E K5 D K4) */
static uint8_t* des_six_decrypt_raw(const uint8_t* key, const uint8_t* data,
                                    const size_t data_len, size_t* output_len)
{
    if ((data_len & 7) != 0 || data_len < 8) return NULL;
    uint8_t *out = s_malloc(data_len);
    if (!out) return NULL;
    memcpy(out, data, data_len);
    for (size_t off = 0; off < data_len; off += 8)
    {
        uint8_t t[8], t2[8], t3[8], t4[8], t5[8], t6[8];
        des_six_decrypt_block(out + off, t, key + 16);
        des_six_encrypt_block(t, t2, key + 8);
        des_six_decrypt_block(t2, t3, key + 0);
        des_six_decrypt_block(t3, t4, key + 40);
        des_six_encrypt_block(t4, t5, key + 32);
        des_six_decrypt_block(t5, t6, key + 24);
        memcpy(out + off, t6, 8);
    }
    *output_len = data_len;
    return out;
}

static char* des_ecb_six_encrypt(cipher_interface_t* self, const char* text)
{
    if(!self || !text) return NULL;
    const des_ecb_six_android_data_t* d = self->private_data;
    const size_t text_len = strlen(text);
    size_t out_len=0; uint8_t* out = des_six_encrypt_raw(d->key, (const uint8_t*)text, text_len, &out_len);
    if(!out) return NULL;
    char* hex = bytes_2_hex(out, out_len);
    s_free(out);
    return hex;
}

static char* des_ecb_six_decrypt(cipher_interface_t* self, const char* hex)
{
    if(!self || !hex) return NULL;
    const des_ecb_six_android_data_t* d = self->private_data;
    size_t bytes_len=0; uint8_t* bytes = hex_2_bytes(hex, &bytes_len);
    if(!bytes) return NULL;
    size_t out_len=0; uint8_t* out = des_six_decrypt_raw(d->key, bytes, bytes_len, &out_len);
    s_free(bytes);
    if(!out) return NULL;
    while(out_len > 0 && out[out_len - 1] == 0) out_len--;
    char* result = s_malloc(out_len + 1);
    memcpy(result, out, out_len);
    result[out_len] = '\0';
    s_free(out);
    return result;
}

static void des_ecb_six_destroy(cipher_interface_t* self)
{
    if(self)
    {
        s_free(self->private_data);
        s_free(self);
    }
}

cipher_interface_t* create_des_ecb_six_cipher(const uint8_t* key)
{
    if(!key) return NULL;
    cipher_interface_t* ci = s_calloc(1, sizeof(cipher_interface_t));
    des_ecb_six_android_data_t* d = s_malloc(sizeof(des_ecb_six_android_data_t));
    memcpy(d->key, key, DES_SIX_KEY_SIZE);
    ci->encrypt = des_ecb_six_encrypt;
    ci->decrypt = des_ecb_six_decrypt;
    ci->destroy = des_ecb_six_destroy;
    ci->private_data = d;
    return ci;
}
