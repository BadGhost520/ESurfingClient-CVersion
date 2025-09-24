#include <stdlib.h>
#include <string.h>

#include "../../headFiles/cipher/impl/MODXTEA.h"

// 从字节数组中获取大端序32位整数
uint32_t modxtea_get_uint32_be(const uint8_t* data, size_t offset) {
    return ((uint32_t)data[offset] << 24) |
           ((uint32_t)data[offset + 1] << 16) |
           ((uint32_t)data[offset + 2] << 8) |
           (uint32_t)data[offset + 3];
}

// 将32位整数以大端序写入字节数组
void modxtea_set_uint32_be(uint8_t* data, size_t offset, uint32_t value) {
    data[offset] = (value >> 24) & 0xFF;
    data[offset + 1] = (value >> 16) & 0xFF;
    data[offset + 2] = (value >> 8) & 0xFF;
    data[offset + 3] = value & 0xFF;
}

// 填充到8字节的倍数
uint8_t* modxtea_pad_to_multiple_of_8(const uint8_t* data, size_t data_len, size_t* padded_len) {
    size_t padding = (8 - (data_len % 8)) % 8;
    *padded_len = data_len + padding;

    uint8_t* padded = calloc(*padded_len, 1);
    if (!padded) return NULL;

    memcpy(padded, data, data_len);
    return padded;
}

// 去除尾部的零填充
uint8_t* modxtea_remove_padding(const uint8_t* data, size_t data_len, size_t* unpadded_len) {
    size_t actual_len = data_len;
    while (actual_len > 0 && data[actual_len - 1] == 0) {
        actual_len--;
    }

    *unpadded_len = actual_len;
    uint8_t* unpadded = malloc(*unpadded_len);
    if (!unpadded) return NULL;

    memcpy(unpadded, data, *unpadded_len);
    return unpadded;
}

// 初始化ModXTEA上下文
void modxtea_init(modxtea_context_t* ctx, const uint32_t* key1, const uint32_t* key2,
                  const uint32_t* key3, const uint32_t* iv) {
    memcpy(ctx->key1, key1, 4 * sizeof(uint32_t));
    memcpy(ctx->key2, key2, 4 * sizeof(uint32_t));
    memcpy(ctx->key3, key3, 4 * sizeof(uint32_t));

    if (iv) {
        memcpy(ctx->iv, iv, 2 * sizeof(uint32_t));
        ctx->has_iv = 1;
    } else {
        ctx->has_iv = 0;
    }
}

// ModXTEA块加密
void modxtea_encrypt_block(uint32_t v0, uint32_t v1, const uint32_t* key,
                          uint32_t* out_v0, uint32_t* out_v1) {
    uint32_t sum = 0;

    for (int i = 0; i < MODXTEA_NUM_ROUNDS; i++) {
        v0 += (v1 ^ sum) + key[sum & 3] + ((v1 << 4) ^ (v1 >> 5));
        sum += MODXTEA_DELTA;
        v1 += key[(sum >> 11) & 3] + (v0 ^ sum) + ((v0 << 4) ^ (v0 >> 5));
    }

    *out_v0 = v0;
    *out_v1 = v1;
}

// ModXTEA块解密
void modxtea_decrypt_block(uint32_t v0, uint32_t v1, const uint32_t* key,
                          uint32_t* out_v0, uint32_t* out_v1) {
    uint32_t sum = MODXTEA_DELTA * MODXTEA_NUM_ROUNDS;

    for (int i = 0; i < MODXTEA_NUM_ROUNDS; i++) {
        v1 -= key[(sum >> 11) & 3] + (v0 ^ sum) + ((v0 << 4) ^ (v0 >> 5));
        sum -= MODXTEA_DELTA;
        v0 -= (v1 ^ sum) + key[sum & 3] + ((v1 << 4) ^ (v1 >> 5));
    }

    *out_v0 = v0;
    *out_v1 = v1;
}

// ModXTEA加密（三轮）
int modxtea_encrypt(const uint32_t* key1, const uint32_t* key2, const uint32_t* key3,
                    const uint8_t* plaintext, size_t plaintext_len,
                    uint8_t** ciphertext, size_t* ciphertext_len) {
    if (!key1 || !key2 || !key3 || !plaintext || !ciphertext || !ciphertext_len) return -1;

    // 填充到8字节对齐
    uint8_t* padded = modxtea_pad_to_multiple_of_8(plaintext, plaintext_len, ciphertext_len);
    if (!padded) return -1;

    *ciphertext = malloc(*ciphertext_len);
    if (!*ciphertext) {
        free(padded);
        return -1;
    }

    // 复制填充后的数据
    memcpy(*ciphertext, padded, *ciphertext_len);

    // 分块处理
    for (size_t i = 0; i < *ciphertext_len; i += MODXTEA_BLOCK_SIZE) {
        uint32_t v0 = modxtea_get_uint32_be(*ciphertext, i);
        uint32_t v1 = modxtea_get_uint32_be(*ciphertext, i + 4);

        // 第一轮加密
        uint32_t round1_v0, round1_v1;
        modxtea_encrypt_block(v0, v1, key1, &round1_v0, &round1_v1);

        // 第二轮加密
        uint32_t round2_v0, round2_v1;
        modxtea_encrypt_block(round1_v0, round1_v1, key2, &round2_v0, &round2_v1);

        // 第三轮加密
        uint32_t round3_v0, round3_v1;
        modxtea_encrypt_block(round2_v0, round2_v1, key3, &round3_v0, &round3_v1);

        // 写回结果
        modxtea_set_uint32_be(*ciphertext, i, round3_v0);
        modxtea_set_uint32_be(*ciphertext, i + 4, round3_v1);
    }

    free(padded);
    return 0;
}

// ModXTEA解密（三轮）
int modxtea_decrypt(const uint32_t* key1, const uint32_t* key2, const uint32_t* key3,
                    const uint8_t* ciphertext, size_t ciphertext_len,
                    uint8_t** plaintext, size_t* plaintext_len) {
    if (!key1 || !key2 || !key3 || !ciphertext || !plaintext || !plaintext_len) return -1;

    uint8_t* decrypted = malloc(ciphertext_len);
    if (!decrypted) return -1;

    // 复制密文数据
    memcpy(decrypted, ciphertext, ciphertext_len);

    // 分块处理
    for (size_t i = 0; i < ciphertext_len; i += MODXTEA_BLOCK_SIZE) {
        uint32_t v0 = modxtea_get_uint32_be(decrypted, i);
        uint32_t v1 = modxtea_get_uint32_be(decrypted, i + 4);

        // 第一轮解密（使用key3）
        uint32_t round1_v0, round1_v1;
        modxtea_decrypt_block(v0, v1, key3, &round1_v0, &round1_v1);

        // 第二轮解密（使用key2）
        uint32_t round2_v0, round2_v1;
        modxtea_decrypt_block(round1_v0, round1_v1, key2, &round2_v0, &round2_v1);

        // 第三轮解密（使用key1）
        uint32_t round3_v0, round3_v1;
        modxtea_decrypt_block(round2_v0, round2_v1, key1, &round3_v0, &round3_v1);

        // 写回结果
        modxtea_set_uint32_be(decrypted, i, round3_v0);
        modxtea_set_uint32_be(decrypted, i + 4, round3_v1);
    }

    // 去除填充
    *plaintext = modxtea_remove_padding(decrypted, ciphertext_len, plaintext_len);
    free(decrypted);

    return (*plaintext != NULL) ? 0 : -1;
}

// ModXTEA with IV加密
int modxtea_encrypt_iv(const uint32_t* key1, const uint32_t* key2, const uint32_t* key3,
                       const uint32_t* iv, const uint8_t* plaintext, size_t plaintext_len,
                       uint8_t** ciphertext, size_t* ciphertext_len) {
    if (!iv) return -1;

    // 先进行普通的ModXTEA加密
    int result = modxtea_encrypt(key1, key2, key3, plaintext, plaintext_len, ciphertext, ciphertext_len);
    if (result != 0) return result;

    // 对每个块与IV进行XOR操作（简化的IV处理）
    for (size_t i = 0; i < *ciphertext_len; i += MODXTEA_BLOCK_SIZE) {
        uint32_t v0 = modxtea_get_uint32_be(*ciphertext, i);
        uint32_t v1 = modxtea_get_uint32_be(*ciphertext, i + 4);

        v0 ^= iv[0];
        v1 ^= iv[1];

        modxtea_set_uint32_be(*ciphertext, i, v0);
        modxtea_set_uint32_be(*ciphertext, i + 4, v1);
    }

    return 0;
}

// ModXTEA with IV解密
int modxtea_decrypt_iv(const uint32_t* key1, const uint32_t* key2, const uint32_t* key3,
                       const uint32_t* iv, const uint8_t* ciphertext, size_t ciphertext_len,
                       uint8_t** plaintext, size_t* plaintext_len) {
    if (!iv) return -1;

    uint8_t* temp_ciphertext = malloc(ciphertext_len);
    if (!temp_ciphertext) return -1;

    memcpy(temp_ciphertext, ciphertext, ciphertext_len);

    // 先对每个块与IV进行XOR操作
    for (size_t i = 0; i < ciphertext_len; i += MODXTEA_BLOCK_SIZE) {
        uint32_t v0 = modxtea_get_uint32_be(temp_ciphertext, i);
        uint32_t v1 = modxtea_get_uint32_be(temp_ciphertext, i + 4);

        v0 ^= iv[0];
        v1 ^= iv[1];

        modxtea_set_uint32_be(temp_ciphertext, i, v0);
        modxtea_set_uint32_be(temp_ciphertext, i + 4, v1);
    }

    // 然后进行普通的ModXTEA解密
    int result = modxtea_decrypt(key1, key2, key3, temp_ciphertext, ciphertext_len, plaintext, plaintext_len);
    free(temp_ciphertext);

    return result;
}