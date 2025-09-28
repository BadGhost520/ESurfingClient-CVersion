#include "../../headFiles/cipher/cipher_interface.h"
#include "../../headFiles/cipher/cipher_utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/**
 * SM4-CBC实现 - 对应Kotlin的SM4CBC类
 * 
 * SM4是中国的分组密码算法，块大小为16字节
 * 这里实现SM4-CBC模式
 */

#define SM4_BLOCK_SIZE 16
#define SM4_KEY_SIZE 16

typedef struct {
    uint8_t key[SM4_KEY_SIZE];
    uint8_t iv[SM4_BLOCK_SIZE];
} sm4_cbc_data_t;

// SM4算法的S盒
static const uint8_t SM4_SBOX[256] = {
    0xd6, 0x90, 0xe9, 0xfe, 0xcc, 0xe1, 0x3d, 0xb7, 0x16, 0xb6, 0x14, 0xc2, 0x28, 0xfb, 0x2c, 0x05,
    0x2b, 0x67, 0x9a, 0x76, 0x2a, 0xbe, 0x04, 0xc3, 0xaa, 0x44, 0x13, 0x26, 0x49, 0x86, 0x06, 0x99,
    0x9c, 0x42, 0x50, 0xf4, 0x91, 0xef, 0x98, 0x7a, 0x33, 0x54, 0x0b, 0x43, 0xed, 0xcf, 0xac, 0x62,
    0xe4, 0xb3, 0x1c, 0xa9, 0xc9, 0x08, 0xe8, 0x95, 0x80, 0xdf, 0x94, 0xfa, 0x75, 0x8f, 0x3f, 0xa6,
    0x47, 0x07, 0xa7, 0xfc, 0xf3, 0x73, 0x17, 0xba, 0x83, 0x59, 0x3c, 0x19, 0xe6, 0x85, 0x4f, 0xa8,
    0x68, 0x6b, 0x81, 0xb2, 0x71, 0x64, 0xda, 0x8b, 0xf8, 0xeb, 0x0f, 0x4b, 0x70, 0x56, 0x9d, 0x35,
    0x1e, 0x24, 0x0e, 0x5e, 0x63, 0x58, 0xd1, 0xa2, 0x25, 0x22, 0x7c, 0x3b, 0x01, 0x21, 0x78, 0x87,
    0xd4, 0x00, 0x46, 0x57, 0x9f, 0xd3, 0x27, 0x52, 0x4c, 0x36, 0x02, 0xe7, 0xa0, 0xc4, 0xc8, 0x9e,
    0xea, 0xbf, 0x8a, 0xd2, 0x40, 0xc7, 0x38, 0xb5, 0xa3, 0xf7, 0xf2, 0xce, 0xf9, 0x61, 0x15, 0xa1,
    0xe0, 0xae, 0x5d, 0xa4, 0x9b, 0x34, 0x1a, 0x55, 0xad, 0x93, 0x32, 0x30, 0xf5, 0x8c, 0xb1, 0xe3,
    0x1d, 0xf6, 0xe2, 0x2e, 0x82, 0x66, 0xca, 0x60, 0xc0, 0x29, 0x23, 0xab, 0x0d, 0x53, 0x4e, 0x6f,
    0xd5, 0xdb, 0x37, 0x45, 0xde, 0xfd, 0x8e, 0x2f, 0x03, 0xff, 0x6a, 0x72, 0x6d, 0x6c, 0x5b, 0x51,
    0x8d, 0x1b, 0xaf, 0x92, 0xbb, 0xdd, 0xbc, 0x7f, 0x11, 0xd9, 0x5c, 0x41, 0x1f, 0x10, 0x5a, 0xd8,
    0x0a, 0xc1, 0x31, 0x88, 0xa5, 0xcd, 0x7b, 0xbd, 0x2d, 0x74, 0xd0, 0x12, 0xb8, 0xe5, 0xb4, 0xb0,
    0x89, 0x69, 0x97, 0x4a, 0x0c, 0x96, 0x77, 0x7e, 0x65, 0xb9, 0xf1, 0x09, 0xc5, 0x6e, 0xc6, 0x84,
    0x18, 0xf0, 0x7d, 0xec, 0x3a, 0xdc, 0x4d, 0x20, 0x79, 0xee, 0x5f, 0x3e, 0xd7, 0xcb, 0x39, 0x48
};

// 常数CK
static const uint32_t SM4_CK[32] = {
    0x00070e15, 0x1c232a31, 0x383f464d, 0x545b6269,
    0x70777e85, 0x8c939aa1, 0xa8afb6bd, 0xc4cbd2d9,
    0xe0e7eef5, 0xfc030a11, 0x181f262d, 0x343b4249,
    0x50575e65, 0x6c737a81, 0x888f969d, 0xa4abb2b9,
    0xc0c7ced5, 0xdce3eaf1, 0xf8ff060d, 0x141b2229,
    0x30373e45, 0x4c535a61, 0x686f767d, 0x848b9299,
    0xa0a7aeb5, 0xbcc3cad1, 0xd8dfe6ed, 0xf4fb0209,
    0x10171e25, 0x2c333a41, 0x484f565d, 0x646b7279
};

// 常数FK
static const uint32_t SM4_FK[4] = {
    0xa3b1bac6, 0x56aa3350, 0x677d9197, 0xb27022dc
};

// 辅助函数：循环左移
static uint32_t ROL(uint32_t x, int n) {
    return (x << n) | (x >> (32 - n));
}

// 非线性变换τ
static uint32_t tau(uint32_t A) {
    return (SM4_SBOX[(A >> 24) & 0xFF] << 24) |
           (SM4_SBOX[(A >> 16) & 0xFF] << 16) |
           (SM4_SBOX[(A >> 8) & 0xFF] << 8) |
           (SM4_SBOX[A & 0xFF]);
}

// 线性变换L
static uint32_t L(uint32_t B) {
    return B ^ ROL(B, 2) ^ ROL(B, 10) ^ ROL(B, 18) ^ ROL(B, 24);
}

// 线性变换L'（用于密钥扩展）
static uint32_t L_prime(uint32_t B) {
    return B ^ ROL(B, 13) ^ ROL(B, 23);
}

// 合成置换T
static uint32_t T(uint32_t X) {
    return L(tau(X));
}

// 合成置换T'（用于密钥扩展）
static uint32_t T_prime(uint32_t X) {
    return L_prime(tau(X));
}

// SM4密钥扩展
static void sm4_key_expansion(const uint8_t* key, uint32_t* rk) {
    uint32_t MK[4];
    uint32_t K[36];
    
    // 将密钥转换为32位字
    for (int i = 0; i < 4; i++) {
        MK[i] = (key[i * 4] << 24) | (key[i * 4 + 1] << 16) |
                (key[i * 4 + 2] << 8) | key[i * 4 + 3];
    }
    
    // 初始化K
    for (int i = 0; i < 4; i++) {
        K[i] = MK[i] ^ SM4_FK[i];
    }
    
    // 生成轮密钥
    for (int i = 0; i < 32; i++) {
        K[i + 4] = K[i] ^ T_prime(K[i + 1] ^ K[i + 2] ^ K[i + 3] ^ SM4_CK[i]);
        rk[i] = K[i + 4];
    }
}

// SM4加密一个块
static void sm4_encrypt_block(const uint8_t* input, uint8_t* output, const uint32_t* rk) {
    uint32_t X[36];
    
    // 将输入转换为32位字
    for (int i = 0; i < 4; i++) {
        X[i] = (input[i * 4] << 24) | (input[i * 4 + 1] << 16) |
               (input[i * 4 + 2] << 8) | input[i * 4 + 3];
    }
    
    // 32轮迭代
    for (int i = 0; i < 32; i++) {
        X[i + 4] = X[i] ^ T(X[i + 1] ^ X[i + 2] ^ X[i + 3] ^ rk[i]);
    }
    
    // 反序变换
    for (int i = 0; i < 4; i++) {
        uint32_t Y = X[35 - i];
        output[i * 4] = (Y >> 24) & 0xFF;
        output[i * 4 + 1] = (Y >> 16) & 0xFF;
        output[i * 4 + 2] = (Y >> 8) & 0xFF;
        output[i * 4 + 3] = Y & 0xFF;
    }
}

// SM4解密一个块
static void sm4_decrypt_block(const uint8_t* input, uint8_t* output, const uint32_t* rk) {
    uint32_t X[36];
    
    // 将输入转换为32位字
    for (int i = 0; i < 4; i++) {
        X[i] = (input[i * 4] << 24) | (input[i * 4 + 1] << 16) |
               (input[i * 4 + 2] << 8) | input[i * 4 + 3];
    }
    
    // 32轮迭代（使用反向轮密钥）
    for (int i = 0; i < 32; i++) {
        X[i + 4] = X[i] ^ T(X[i + 1] ^ X[i + 2] ^ X[i + 3] ^ rk[31 - i]);
    }
    
    // 反序变换
    for (int i = 0; i < 4; i++) {
        uint32_t Y = X[35 - i];
        output[i * 4] = (Y >> 24) & 0xFF;
        output[i * 4 + 1] = (Y >> 16) & 0xFF;
        output[i * 4 + 2] = (Y >> 8) & 0xFF;
        output[i * 4 + 3] = Y & 0xFF;
    }
}

// SM4-CBC加密
static uint8_t* sm4_encrypt_cbc(const uint8_t* key, const uint8_t* iv,
                                const uint8_t* plaintext, size_t plaintext_len,
                                size_t* output_len) {
    // PKCS7填充
    size_t padded_len;
    uint8_t* padded_data = pkcs7_padding(plaintext, plaintext_len, SM4_BLOCK_SIZE, &padded_len);
    if (!padded_data) return NULL;

    size_t blocks = padded_len / SM4_BLOCK_SIZE;
    uint8_t* output = safe_malloc(padded_len);

    // 密钥扩展
    uint32_t rk[32];
    sm4_key_expansion(key, rk);

    // 初始化前一个块为IV
    uint8_t prev_block[SM4_BLOCK_SIZE];
    memcpy(prev_block, iv, SM4_BLOCK_SIZE);

    // 逐块加密
    for (size_t i = 0; i < blocks; i++) {
        uint8_t block[SM4_BLOCK_SIZE];
        uint8_t xored_block[SM4_BLOCK_SIZE];

        // 获取当前块
        memcpy(block, padded_data + i * SM4_BLOCK_SIZE, SM4_BLOCK_SIZE);

        // 与前一个密文块异或
        xor_bytes(block, prev_block, xored_block, SM4_BLOCK_SIZE);

        // SM4加密
        sm4_encrypt_block(xored_block, output + i * SM4_BLOCK_SIZE, rk);

        // 更新前一个块
        memcpy(prev_block, output + i * SM4_BLOCK_SIZE, SM4_BLOCK_SIZE);
    }

    safe_free(padded_data);
    *output_len = padded_len;
    return output;
}

// SM4-CBC解密
static uint8_t* sm4_decrypt_cbc(const uint8_t* key, const uint8_t* iv,
                                const uint8_t* ciphertext, size_t ciphertext_len,
                                size_t* output_len) {
    if (ciphertext_len % SM4_BLOCK_SIZE != 0) return NULL;

    size_t blocks = ciphertext_len / SM4_BLOCK_SIZE;
    uint8_t* output = safe_malloc(ciphertext_len);

    // 密钥扩展
    uint32_t rk[32];
    sm4_key_expansion(key, rk);

    // 初始化前一个块为IV
    uint8_t prev_block[SM4_BLOCK_SIZE];
    memcpy(prev_block, iv, SM4_BLOCK_SIZE);

    // 逐块解密
    for (size_t i = 0; i < blocks; i++) {
        uint8_t block[SM4_BLOCK_SIZE];
        uint8_t decrypted_block[SM4_BLOCK_SIZE];

        // 获取当前密文块
        memcpy(block, ciphertext + i * SM4_BLOCK_SIZE, SM4_BLOCK_SIZE);

        // SM4解密
        sm4_decrypt_block(block, decrypted_block, rk);

        // 与前一个密文块异或（注意：这里应该是与前一个密文块异或，不是解密后的块）
        xor_bytes(decrypted_block, prev_block, output + i * SM4_BLOCK_SIZE, SM4_BLOCK_SIZE);

        // 更新前一个块为当前密文块
        memcpy(prev_block, block, SM4_BLOCK_SIZE);
    }

    // PKCS7去填充
    size_t unpadded_len;
    uint8_t* unpadded_data = remove_pkcs7_padding(output, ciphertext_len, &unpadded_len);
    safe_free(output);

    *output_len = unpadded_len;
    return unpadded_data;
}

// 加密实现 - 对应Kotlin: override fun encrypt(text: String): String
static char* sm4_cbc_encrypt(cipher_interface_t* self, const char* text) {
    if (!self || !text) return NULL;

    sm4_cbc_data_t* data = (sm4_cbc_data_t*)self->private_data;
    if (!data) return NULL;

    size_t text_len = strlen(text);
    size_t output_len;

    // SM4-CBC加密
    uint8_t* encrypted = sm4_encrypt_cbc(data->key, data->iv,
                                         (const uint8_t*)text, text_len,
                                         &output_len);
    if (!encrypted) return NULL;

    // 转换为大写十六进制字符串
    char* hex_result = bytes_to_hex_upper(encrypted, output_len);
    safe_free(encrypted);

    return hex_result;
}

// 解密实现 - 对应Kotlin: override fun decrypt(hex: String): String
static char* sm4_cbc_decrypt(cipher_interface_t* self, const char* hex) {
    if (!self || !hex) return NULL;

    sm4_cbc_data_t* data = (sm4_cbc_data_t*)self->private_data;
    if (!data) return NULL;

    // 将十六进制字符串转换为字节数组
    size_t bytes_len;
    uint8_t* bytes = hex_to_bytes(hex, &bytes_len);
    if (!bytes) return NULL;

    size_t output_len;

    // SM4-CBC解密
    uint8_t* decrypted = sm4_decrypt_cbc(data->key, data->iv, bytes, bytes_len, &output_len);
    safe_free(bytes);

    if (!decrypted) return NULL;

    // 与Kotlin版本一致：去除尾部零字节（dropLastWhile { it == 0.toByte() }）
    while (output_len > 0 && decrypted[output_len - 1] == 0) {
        output_len--;
    }

    // 转换为字符串
    char* result = safe_malloc(output_len + 1);
    memcpy(result, decrypted, output_len);
    result[output_len] = '\0';
    safe_free(decrypted);
    
    return result;
}

// 销毁函数
static void sm4_cbc_destroy(cipher_interface_t* self) {
    if (self) {
        safe_free(self->private_data);
        safe_free(self);
    }
}

// 创建SM4-CBC加解密实例
cipher_interface_t* create_sm4_cbc_cipher(const uint8_t* key, const uint8_t* iv) {
    if (!key || !iv) return NULL;
    
    cipher_interface_t* cipher = safe_malloc(sizeof(cipher_interface_t));
    sm4_cbc_data_t* data = safe_malloc(sizeof(sm4_cbc_data_t));
    
    // 复制密钥和IV
    memcpy(data->key, key, SM4_KEY_SIZE);
    memcpy(data->iv, iv, SM4_BLOCK_SIZE);
    
    // 设置函数指针
    cipher->encrypt = sm4_cbc_encrypt;
    cipher->decrypt = sm4_cbc_decrypt;
    cipher->destroy = sm4_cbc_destroy;
    cipher->private_data = data;
    
    return cipher;
}