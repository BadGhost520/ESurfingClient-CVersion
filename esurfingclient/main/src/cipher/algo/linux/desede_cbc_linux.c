#include "cipher/CipherInterface.h"
#include "cipher/CipherUtils.h"

#include "utils/simssl/evp.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef struct {
    uint8_t key1[24];
    uint8_t key2[24];
    uint8_t iv1[8];
    uint8_t iv2[8];
} desede_cbc_linux_data_t;

static void xor8(uint8_t *dst, const uint8_t *src)
{
  for (int i = 0; i < 8; ++i) dst[i] ^= src[i];
}

static void stage_encrypt(uint8_t *buf, const size_t len,
                          const uint8_t key24[24],
                          const uint8_t iv[8])
{
  uint8_t prev[8];
  memcpy(prev, iv, 8);
  for (size_t off = 0; off < len; off += 8)
  {
    uint8_t blk[8];
    memcpy(blk, buf + off, 8);
    xor8(blk, prev);
    simssl_des3_encrypt_block(key24, blk, buf + off);
    memcpy(prev, buf + off, 8);
  }
}

static void stage_decrypt(uint8_t *buf, const size_t len,
                          const uint8_t key24[24],
                          const uint8_t iv[8])
{
  uint8_t prev[8];
  memcpy(prev, iv, 8);
  for (size_t off = 0; off < len; off += 8)
    {
    uint8_t cblk[8], pblk[8];
    memcpy(cblk, buf + off, 8);
    simssl_des3_decrypt_block(key24, cblk, pblk);
    xor8(pblk, prev);
    memcpy(buf + off, pblk, 8);
    memcpy(prev, cblk, 8);
  }
}

static char* desede_cbc_encrypt(cipher_interface_t* self, const char* text)
{
  if (!self || !text) return NULL;
  desede_cbc_linux_data_t* data = self->private_data;
  if (!data) return NULL;
  const size_t text_len = strlen(text);
  size_t padded_len;
  uint8_t* padded = pad_2_multiple((const uint8_t*)text, text_len, 8, &padded_len);
  if (!padded) return NULL;
  stage_encrypt(padded, padded_len, data->key2, data->iv2);
  stage_encrypt(padded, padded_len, data->key1, data->iv1);
  char* hex = bytes_2_hex(padded, padded_len);
  s_free(padded);
  return hex;
}

static char* desede_cbc_decrypt(cipher_interface_t* self, const char* hex)
{
  if (!self || !hex) return NULL;
  desede_cbc_linux_data_t* data = self->private_data;
  if (!data) return NULL;
  size_t bytes_len;
  uint8_t* bytes = hex_2_bytes(hex, &bytes_len);
  if (!bytes) return NULL;
  stage_decrypt(bytes, bytes_len, data->key1, data->iv1);
  stage_decrypt(bytes, bytes_len, data->key2, data->iv2);
  while (bytes_len > 0 && bytes[bytes_len - 1] == 0)
  {
    bytes_len--;
  }
  char* result = s_malloc(bytes_len + 1);
  memcpy(result, bytes, bytes_len);
  result[bytes_len] = '\0';
  s_free(bytes);
  return result;
}

static void desede_cbc_destroy(cipher_interface_t* self)
{
  if (self)
  {
    s_free(self->private_data);
    s_free(self);
  }
}

cipher_interface_t* create_desede_cbc_linux_cipher(const uint8_t* key1, const uint8_t* key2,
                                                const uint8_t* iv1, const uint8_t* iv2)
{
  if (!key1 || !key2 || !iv1 || !iv2) return NULL;
  cipher_interface_t* c = s_malloc(sizeof(cipher_interface_t));
  desede_cbc_linux_data_t* d = s_malloc(sizeof(desede_cbc_linux_data_t));
  memcpy(d->key1, key1, 24);
  memcpy(d->key2, key2, 24);
  memcpy(d->iv1, iv1, 8);
  memcpy(d->iv2, iv2, 8);
  c->encrypt = desede_cbc_encrypt;
  c->decrypt = desede_cbc_decrypt;
  c->destroy = desede_cbc_destroy;
  c->private_data = d;
  return c;
}