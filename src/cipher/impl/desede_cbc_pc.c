#include "../../headFiles/cipher/cipher_interface.h"
#include "../../headFiles/cipher/cipher_utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/**
 * 自实现 3DES-CBC(两层) - 与 decoder.c 保持一致的 EDE 流程
 * 第一层与第二层均为 CBC：每块先异或前一密文块(或IV)，再执行 E(K1) -> D(K2) -> E(K3)
 * 解密按逆序执行：D(K3) -> E(K2) -> D(K1)，再与前一密文块(或IV)异或
 *
 * 本实现不依赖 OpenSSL，使用内置 DES 例程。
 */

typedef struct {
    uint8_t key1[24]; // 3DES Key1 由3个8字节子钥拼接
    uint8_t key2[24]; // 3DES Key2 由3个8字节子钥拼接
    uint8_t iv1[8];   // 第一层 CBC IV
    uint8_t iv2[8];   // 第二层 CBC IV
} desede_cbc_pc_data_t;

// ---- DES 常量表 ----
static const int IP[64] = {
  58,50,42,34,26,18,10,2,60,52,44,36,28,20,12,4,62,54,46,38,30,22,14,6,64,56,48,40,32,24,16,8,57,49,41,33,25,17,9,1,59,51,43,35,27,19,11,3,61,53,45,37,29,21,13,5,63,55,47,39,31,23,15,7
};
static const int FP_[64] = {
  40,8,48,16,56,24,64,32,39,7,47,15,55,23,63,31,38,6,46,14,54,22,62,30,37,5,45,13,53,21,61,29,36,4,44,12,52,20,60,28,35,3,43,11,51,19,59,27,34,2,42,10,50,18,58,26,33,1,41,9,49,17,57,25
};
static const int PC1[56] = {
  57,49,41,33,25,17,9,1,58,50,42,34,26,18,10,2,59,51,43,35,27,19,11,3,60,52,44,36,63,55,47,39,31,23,15,7,62,54,46,38,30,22,14,6,61,53,45,37,29,21,13,5,28,20,12,4
};
static const int E_SELECT[48] = {
  32,1,2,3,4,5,4,5,6,7,8,9,8,9,10,11,12,13,12,13,14,15,16,17,16,17,18,19,20,21,20,21,22,23,24,25,24,25,26,27,28,29,28,29,30,31,32,1
};
static const int PC2_SELECT[48] = {
  14,17,11,24,1,5,3,28,15,6,21,10,23,19,12,4,26,8,16,7,27,20,13,2,41,52,31,37,47,55,30,40,51,45,33,48,44,49,39,56,34,53,46,42,50,36,29,32
};
static const int P_[32] = {
  16,7,20,21,29,12,28,17,1,15,23,26,5,18,31,10,2,8,24,14,32,27,3,9,19,13,30,6,22,11,4,25
};
static const uint8_t SBOX[8*64] = {
  14,4,13,1,2,15,11,8,3,10,6,12,5,9,0,7,0,15,7,4,14,2,13,1,10,6,12,11,9,5,3,8,4,1,14,8,13,6,2,11,15,12,9,7,3,10,5,0,15,12,8,2,4,9,1,7,5,11,3,14,10,0,6,13,
  15,1,8,14,6,11,3,4,9,7,2,13,12,0,5,10,3,13,4,7,15,2,8,14,12,0,1,10,6,9,11,5,0,14,7,11,10,4,13,1,5,8,12,6,9,3,2,15,13,8,10,1,3,15,4,2,11,6,7,12,0,5,14,9,
  10,0,9,14,6,3,15,5,1,13,12,7,11,4,2,8,13,7,0,9,3,4,6,10,2,8,5,14,12,11,15,1,13,6,4,9,8,15,3,0,11,1,2,12,5,10,14,7,1,10,13,0,6,9,8,7,4,15,14,3,11,5,2,12,
  7,13,14,3,0,6,9,10,1,2,8,5,11,12,4,15,13,8,11,5,6,15,0,3,4,7,2,12,1,10,14,9,10,6,9,0,12,11,7,13,15,1,3,14,5,2,8,4,3,15,0,6,10,1,13,8,9,4,5,11,12,7,2,14,
  2,12,4,1,7,10,11,6,8,5,3,15,13,0,14,9,14,11,2,12,4,7,13,1,5,0,15,10,3,9,8,6,4,2,1,11,10,13,7,8,15,9,12,5,6,3,0,14,11,8,12,7,1,14,2,13,6,15,0,9,10,4,5,3,
  12,1,10,15,9,2,6,8,0,13,3,4,14,7,5,11,10,15,4,2,7,12,9,5,6,1,13,14,0,11,3,8,9,14,15,5,2,8,12,3,7,0,4,10,1,13,11,6,4,3,2,12,9,5,15,10,11,14,1,7,6,0,8,13,
  4,11,2,14,15,0,8,13,3,12,9,7,5,10,6,1,13,0,11,7,4,9,1,10,14,3,5,12,2,15,8,6,1,4,11,13,12,3,7,14,10,15,6,8,0,5,9,2,6,11,13,8,1,4,10,7,9,5,0,15,14,2,3,12,
  13,2,8,4,6,15,11,1,10,9,3,14,5,0,12,7,1,15,13,8,10,3,7,4,12,5,6,11,0,14,9,2,7,11,4,1,9,12,14,2,0,6,10,13,15,3,5,8,2,1,14,7,4,10,8,13,15,12,9,0,3,5,6,11
};
static const int SHIFTS[16] = {1,1,2,2,2,2,2,2,1,2,2,2,2,2,2,1};

// ---- DES 工具函数 ----
static void bytes_to_bits(const uint8_t *in, int nbytes, uint8_t *out_bits) {
  for (int b = 0; b < nbytes; ++b) {
    uint8_t v = in[b];
    for (int i = 7; i >= 0; --i) {
      *out_bits++ = (v >> i) & 1u;
    }
  }
}

static void bits_to_bytes(const uint8_t *in_bits, int nbytes, uint8_t *out) {
  for (int b = 0; b < nbytes; ++b) {
    uint8_t v = 0;
    for (int i = 0; i < 8; ++i) {
      v = (v << 1) | (in_bits[b*8 + i] & 1u);
    }
    out[b] = v;
  }
}

static void apply_perm(uint8_t *dst_bits, const uint8_t *src_bits, const int *table, int count) {
  for (int i = 0; i < count; ++i) {
    dst_bits[i] = src_bits[table[i] - 1] & 1u; // 1-based
  }
}

static void rotate_cd(uint8_t *cd56, int shift) {
  uint8_t C[28], D[28];
  memcpy(C, cd56, 28);
  memcpy(D, cd56 + 28, 28);
  for (int i = 0; i < 28; ++i) {
    cd56[i]      = C[(i + shift) % 28];
    cd56[28 + i] = D[(i + shift) % 28];
  }
}

static void des_round_f(const uint8_t R[32], const uint8_t subkey48[48], uint8_t out32[32]) {
  uint8_t expanded[48];
  apply_perm(expanded, R, E_SELECT, 48);
  for (int i = 0; i < 48; ++i) expanded[i] ^= subkey48[i];
  uint8_t s_out[32];
  const uint8_t *p = expanded;
  for (int box = 0; box < 8; ++box) {
    int idx = (box << 6) + p[4] + 2 * (p[3] + 2 * (p[2] + 2 * (p[1] + 2 * (p[5] + 2 * p[0]))));
    uint8_t val = SBOX[idx] & 0x0F;
    s_out[box*4 + 0] = (val >> 3) & 1;
    s_out[box*4 + 1] = (val >> 2) & 1;
    s_out[box*4 + 2] = (val >> 1) & 1;
    s_out[box*4 + 3] = (val >> 0) & 1;
    p += 6;
  }
  apply_perm(out32, s_out, P_, 32);
}

static void des_key_schedule(const uint8_t key8[8], uint8_t subkeys[16][48]) {
  uint8_t key_bits[64];
  uint8_t cd[56];
  uint8_t tmp48[48];
  bytes_to_bits(key8, 8, key_bits);
  apply_perm(cd, key_bits, PC1, 56);
  for (int round = 0; round < 16; ++round) {
    rotate_cd(cd, SHIFTS[round]);
    apply_perm(tmp48, cd, PC2_SELECT, 48);
    memcpy(subkeys[round], tmp48, 48);
  }
}

static void des_block_process(const uint8_t in[8], uint8_t out[8], const uint8_t subkeys[16][48], int encrypt) {
  uint8_t in_bits[64];
  uint8_t ip[64];
  bytes_to_bits(in, 8, in_bits);
  apply_perm(ip, in_bits, IP, 64);
  uint8_t L[32], R[32];
  memcpy(L, ip, 32);
  memcpy(R, ip + 32, 32);
  for (int round = 0; round < 16; ++round) {
    uint8_t f[32];
    const uint8_t *sk = encrypt ? subkeys[round] : subkeys[15 - round];
    des_round_f(R, sk, f);
    uint8_t newR[32];
    for (int i = 0; i < 32; ++i) newR[i] = L[i] ^ f[i];
    memcpy(L, R, 32);
    memcpy(R, newR, 32);
  }
  uint8_t preout[64];
  memcpy(preout, R, 32);
  memcpy(preout + 32, L, 32);
  uint8_t out_bits[64];
  apply_perm(out_bits, preout, FP_, 64);
  bits_to_bytes(out_bits, 8, out);
}

static void des_encrypt_block(const uint8_t in[8], uint8_t out[8], const uint8_t key[8]) {
  uint8_t subkeys[16][48];
  des_key_schedule(key, subkeys);
  des_block_process(in, out, subkeys, 1);
}

static void des_decrypt_block(const uint8_t in[8], uint8_t out[8], const uint8_t key[8]) {
  uint8_t subkeys[16][48];
  des_key_schedule(key, subkeys);
  des_block_process(in, out, subkeys, 0);
}

// XOR 8字节工具
static inline void xor8(uint8_t *dst, const uint8_t *src) {
  for (int i = 0; i < 8; ++i) dst[i] ^= src[i];
}

// 单层 CBC 加密: E(K1)->D(K2)->E(K3)
static void stage_encrypt(uint8_t *buf, size_t len,
                          const uint8_t Kx1[8], const uint8_t Kx2[8], const uint8_t Kx3[8],
                          const uint8_t iv[8]) {
  uint8_t prev[8];
  memcpy(prev, iv, 8);
  for (size_t off = 0; off < len; off += 8) {
    uint8_t blk[8], t1[8], t2[8];
    memcpy(blk, buf + off, 8);
    xor8(blk, prev);
    des_encrypt_block(blk, t1, Kx1);
    des_decrypt_block(t1, t2, Kx2);
    des_encrypt_block(t2, buf + off, Kx3);
    memcpy(prev, buf + off, 8);
  }
}

// 单层 CBC 解密: D(K3)->E(K2)->D(K1)
static void stage_decrypt(uint8_t *buf, size_t len,
                          const uint8_t Kx1[8], const uint8_t Kx2[8], const uint8_t Kx3[8],
                          const uint8_t iv[8]) {
  uint8_t prev[8];
  memcpy(prev, iv, 8);
  for (size_t off = 0; off < len; off += 8) {
    uint8_t cblk[8], t1[8], t2[8], pblk[8];
    memcpy(cblk, buf + off, 8);
    des_decrypt_block(cblk, t1, Kx3);
    des_encrypt_block(t1, t2, Kx2);
    des_decrypt_block(t2, pblk, Kx1);
    xor8(pblk, prev);
    memcpy(buf + off, pblk, 8);
    memcpy(prev, cblk, 8);
  }
}

// 加密实现 - 文本 -> HEX(大写)
static char* desede_cbc_pc_encrypt(cipher_interface_t* self, const char* text) {
  if (!self || !text) return NULL;
  desede_cbc_pc_data_t* data = (desede_cbc_pc_data_t*)self->private_data;
  if (!data) return NULL;

  size_t text_len = strlen(text);
  size_t padded_len;
  uint8_t* padded = pad_to_multiple((const uint8_t*)text, text_len, 8, &padded_len);
  if (!padded) return NULL;

  // 两层加密：先 key2/iv2，再 key1/iv1
  stage_encrypt(padded, padded_len, data->key2 + 0, data->key2 + 8, data->key2 + 16, data->iv2);
  stage_encrypt(padded, padded_len, data->key1 + 0, data->key1 + 8, data->key1 + 16, data->iv1);

  char* hex = bytes_to_hex_upper(padded, padded_len);
  safe_free(padded);
  return hex;
}

// 解密实现 - HEX -> 明文字符串
static char* desede_cbc_pc_decrypt(cipher_interface_t* self, const char* hex) {
  if (!self || !hex) return NULL;
  desede_cbc_pc_data_t* data = (desede_cbc_pc_data_t*)self->private_data;
  if (!data) return NULL;

  size_t bytes_len;
  uint8_t* bytes = hex_to_bytes(hex, &bytes_len);
  if (!bytes) return NULL;

  // 两层解密：先 key1/iv1，再 key2/iv2（与加密顺序相反）
  stage_decrypt(bytes, bytes_len, data->key1 + 0, data->key1 + 8, data->key1 + 16, data->iv1);
  stage_decrypt(bytes, bytes_len, data->key2 + 0, data->key2 + 8, data->key2 + 16, data->iv2);

  // 去除尾部零字节（保持与现有实现一致）
  while (bytes_len > 0 && bytes[bytes_len - 1] == 0) {
    bytes_len--;
  }

  char* result = safe_malloc(bytes_len + 1);
  memcpy(result, bytes, bytes_len);
  result[bytes_len] = '\0';
  safe_free(bytes);
  return result;
}

static void desede_cbc_pc_destroy(cipher_interface_t* self) {
  if (self) {
    safe_free(self->private_data);
    safe_free(self);
  }
}

// 创建实例：复制两组24字节Key与IV
cipher_interface_t* create_desede_cbc_pc_cipher(const uint8_t* key1, const uint8_t* key2,
                                                const uint8_t* iv1, const uint8_t* iv2) {
  if (!key1 || !key2 || !iv1 || !iv2) return NULL;
  cipher_interface_t* c = safe_malloc(sizeof(cipher_interface_t));
  desede_cbc_pc_data_t* d = safe_malloc(sizeof(desede_cbc_pc_data_t));
  memcpy(d->key1, key1, 24);
  memcpy(d->key2, key2, 24);
  memcpy(d->iv1, iv1, 8);
  memcpy(d->iv2, iv2, 8);
  c->encrypt = desede_cbc_pc_encrypt;
  c->decrypt = desede_cbc_pc_decrypt;
  c->destroy = desede_cbc_pc_destroy;
  c->private_data = d;
  return c;
}