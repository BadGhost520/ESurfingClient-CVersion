#include "../../headFiles/cipher/cipher_interface.h"
#include "../../headFiles/cipher/cipher_utils.h"
#include <string.h>
#include <stdint.h>

/**
 * ZUC-128实现 - 对应Kotlin的ZUC类
 * 
 * ZUC-128是中国的流密码算法，用于4G/5G通信加密
 * 这里实现基本的ZUC-128算法
 */

typedef struct {
    uint8_t key[16];   // ZUC-128密钥长度为16字节
    uint8_t iv[16];    // ZUC-128 IV长度为16字节
} zuc_data_t;

// ZUC-128算法的S盒
static const uint8_t S0[256] = {
    0x3e, 0x72, 0x5b, 0x47, 0xca, 0xe0, 0x00, 0x33, 0x04, 0xd1, 0x54, 0x98, 0x09, 0xb9, 0x6d, 0xcb,
    0x7b, 0x1b, 0xf9, 0x32, 0xaf, 0x9d, 0x6a, 0xa5, 0xb8, 0x2d, 0xfc, 0x1d, 0x08, 0x53, 0x03, 0x90,
    0x4d, 0x4e, 0x84, 0x99, 0xe4, 0xce, 0xd9, 0x91, 0xdd, 0xb6, 0x85, 0x48, 0x8b, 0x29, 0x6e, 0xac,
    0xcd, 0xc1, 0xf8, 0x1e, 0x73, 0x43, 0x69, 0xc6, 0xb5, 0xbd, 0xfd, 0x39, 0x63, 0x20, 0xd4, 0x38,
    0x76, 0x7d, 0xb2, 0xa7, 0xcf, 0xed, 0x57, 0xc5, 0xf3, 0x2c, 0xbb, 0x14, 0x21, 0x06, 0x55, 0x9b,
    0xe3, 0xef, 0x5e, 0x31, 0x4f, 0x7f, 0x5a, 0xa4, 0x0d, 0x82, 0x51, 0x49, 0x5f, 0xba, 0x58, 0x1c,
    0x4a, 0x16, 0xd5, 0x17, 0xa8, 0x92, 0x24, 0x1f, 0x8c, 0xff, 0xd8, 0xae, 0x2e, 0x01, 0xd3, 0xad,
    0x3b, 0x4b, 0xda, 0x46, 0xeb, 0xc9, 0xde, 0x9a, 0x8f, 0x87, 0xd7, 0x3a, 0x80, 0x6f, 0x2f, 0xc8,
    0xb1, 0xb4, 0x37, 0xf7, 0x0a, 0x22, 0x13, 0x28, 0x7c, 0xcc, 0x3c, 0x89, 0xc7, 0xc3, 0x96, 0x56,
    0x07, 0xbf, 0x7e, 0xf0, 0x0b, 0x2b, 0x97, 0x52, 0x35, 0x41, 0x79, 0x61, 0xa6, 0x4c, 0x10, 0xfe,
    0xbc, 0x26, 0x95, 0x88, 0x8a, 0xb0, 0xa3, 0xfb, 0xc0, 0x18, 0x94, 0xf2, 0xe1, 0xe5, 0xe9, 0x5d,
    0xd0, 0xdc, 0x11, 0x66, 0x64, 0x5c, 0xec, 0x59, 0x42, 0x75, 0x12, 0xf5, 0x74, 0x9c, 0xaa, 0x23,
    0x0e, 0x86, 0xab, 0xbe, 0x2a, 0x02, 0xe7, 0x67, 0xe6, 0x44, 0xa2, 0x6c, 0xc2, 0x93, 0x9f, 0xf1,
    0xf6, 0xfa, 0x36, 0xd2, 0x50, 0x68, 0x9e, 0x62, 0x71, 0x15, 0x3d, 0xd6, 0x40, 0xc4, 0xe2, 0x0f,
    0x8e, 0x83, 0x77, 0x6b, 0x25, 0x05, 0x3f, 0x0c, 0x30, 0xea, 0x70, 0xb7, 0xa1, 0xe8, 0xa9, 0x65,
    0x8d, 0x27, 0x1a, 0xdb, 0x81, 0xb3, 0xa0, 0xf4, 0x45, 0x7a, 0x19, 0xdf, 0xee, 0x78, 0x34, 0x60
};

static const uint8_t S1[256] = {
    0x55, 0xc2, 0x63, 0x71, 0x3b, 0xc8, 0x47, 0x86, 0x9f, 0x3c, 0xda, 0x5b, 0x29, 0xaa, 0xfd, 0x77,
    0x8c, 0xc5, 0x94, 0x0c, 0xa6, 0x1a, 0x13, 0x00, 0xe3, 0xa8, 0x16, 0x72, 0x40, 0xf9, 0xf8, 0x42,
    0x44, 0x26, 0x68, 0x96, 0x81, 0xd9, 0x45, 0x3e, 0x10, 0x76, 0xc6, 0xa7, 0x8b, 0x39, 0x43, 0xe1,
    0x3a, 0xb5, 0x56, 0x2a, 0xc0, 0x6d, 0xb3, 0x05, 0x22, 0x66, 0xbf, 0xdc, 0x0b, 0xfa, 0x62, 0x48,
    0xdd, 0x20, 0x11, 0x06, 0x36, 0xc9, 0xc1, 0xcf, 0xf6, 0x27, 0x52, 0xbb, 0x69, 0xf5, 0xd4, 0x87,
    0x7f, 0x84, 0x4c, 0xd2, 0x9c, 0x57, 0xa4, 0xbc, 0x4f, 0x9a, 0xdf, 0xfe, 0xd6, 0x8d, 0x7a, 0xeb,
    0x2b, 0x53, 0xd8, 0x5c, 0xa1, 0x14, 0x17, 0xfb, 0x23, 0xd5, 0x7d, 0x30, 0x67, 0x73, 0x08, 0x09,
    0xee, 0xb7, 0x70, 0x3f, 0x61, 0xb2, 0x19, 0x8e, 0x4e, 0xe5, 0x4b, 0x93, 0x8f, 0x5d, 0xdb, 0xa9,
    0xad, 0xf1, 0xae, 0x2e, 0xcb, 0x0d, 0xfc, 0xf4, 0x2d, 0x46, 0x6e, 0x1d, 0x97, 0xe8, 0xd1, 0xe9,
    0x4d, 0x37, 0xa5, 0x75, 0x5e, 0x83, 0x9e, 0xab, 0x82, 0x9d, 0xb9, 0x1c, 0xe0, 0xcd, 0x49, 0x89,
    0x01, 0xb6, 0xbd, 0x58, 0x24, 0xa2, 0x5f, 0x38, 0x78, 0x99, 0x15, 0x90, 0x50, 0xb8, 0x95, 0xe4,
    0xd0, 0x91, 0xc7, 0xce, 0xed, 0x0f, 0xb4, 0x6f, 0xa0, 0xcc, 0xf0, 0x02, 0x4a, 0x79, 0xc3, 0xde,
    0xa3, 0xef, 0xea, 0x51, 0xe6, 0x6b, 0x18, 0xec, 0x1b, 0x2c, 0x80, 0xf7, 0x74, 0xe7, 0xff, 0x21,
    0x5a, 0x6a, 0x54, 0x1e, 0x41, 0x31, 0x92, 0x35, 0xc4, 0x33, 0x07, 0x0a, 0xba, 0x7e, 0x0e, 0x34,
    0x88, 0xb1, 0x98, 0x7c, 0xf3, 0x3d, 0x60, 0x6c, 0x7b, 0xca, 0xd3, 0x1f, 0x32, 0x65, 0x04, 0x28,
    0x64, 0xbe, 0x85, 0x9b, 0x2f, 0x59, 0x8a, 0xd7, 0xb0, 0x25, 0xac, 0xaf, 0x12, 0x03, 0xe2, 0xf2
};

// 常数D - 与Java版本EK_d一致
static const uint16_t EK_d[16] = {
    0x44D7, 0x26BC, 0x626B, 0x135E, 0x5789, 0x35E2, 0x7135, 0x09AF,
    0x4D78, 0x2F13, 0x6BC4, 0x1AF1, 0x5E26, 0x3C4D, 0x789A, 0x47AC
};

// ZUC-128状态结构
typedef struct {
    uint32_t LFSR[16];  // 线性反馈移位寄存器
    uint32_t R1, R2;    // 非线性函数寄存器
    uint32_t X0, X1, X2, X3;  // 工作变量
} zuc_state_t;

// 辅助函数：循环左移
static uint32_t ROL(uint32_t x, int n) {
    return (x << n) | (x >> (32 - n));
}

// 辅助函数：字节替换
static uint32_t S(uint32_t x) {
    return (S0[x >> 24] << 24) | (S1[(x >> 16) & 0xFF] << 16) |
           (S0[(x >> 8) & 0xFF] << 8) | (S1[x & 0xFF]);
}

// 线性变换L1
static uint32_t L1(uint32_t x) {
    return x ^ ROL(x, 2) ^ ROL(x, 10) ^ ROL(x, 18) ^ ROL(x, 24);
}

// 线性变换L2
static uint32_t L2(uint32_t x) {
    return x ^ ROL(x, 8) ^ ROL(x, 14) ^ ROL(x, 22) ^ ROL(x, 30);
}

// 构造32位整数 - 与Java版本MAKEU32一致
static uint32_t MAKEU32(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    return ((a & 0xFF) << 24) | ((b & 0xFF) << 16) | ((c & 0xFF) << 8) | (d & 0xFF);
}

// 构造31位整数 - 与Java版本MAKEU31一致
static uint32_t MAKEU31(uint8_t a, uint16_t b, uint8_t c) {
    return ((a & 0xFF) << 23) | ((b & 0xFFFF) << 8) | (c & 0xFF);
}

// 模运算加法 c = a + b mod (2^31 - 1)
static uint32_t AddM(uint32_t a, uint32_t b) {
    uint32_t c = a + b;
    return (c & 0x7FFFFFFF) + (c >> 31);
}

// 乘以2的k次方 mod (2^31 - 1)
static uint32_t MulByPow2(uint32_t x, int k) {
    return ((x << k) | (x >> (31 - k))) & 0x7FFFFFFF;
}

// BitReorganization - 与Java版本一致
static void BitReorganization(zuc_state_t* state) {
    state->X0 = ((state->LFSR[15] & 0x7FFF8000) << 1) | (state->LFSR[14] & 0xFFFF);
    state->X1 = ((state->LFSR[11] & 0xFFFF) << 16) | (state->LFSR[9] >> 15);
    state->X2 = ((state->LFSR[7] & 0xFFFF) << 16) | (state->LFSR[5] >> 15);
    state->X3 = ((state->LFSR[2] & 0xFFFF) << 16) | (state->LFSR[0] >> 15);
}

// LFSR初始化模式更新
static void LFSRWithInitialisationMode(zuc_state_t* state, uint32_t u) {
    uint32_t f = state->LFSR[0];
    uint32_t v = MulByPow2(state->LFSR[0], 8);
    f = AddM(f, v);
    v = MulByPow2(state->LFSR[4], 20);
    f = AddM(f, v);
    v = MulByPow2(state->LFSR[10], 21);
    f = AddM(f, v);
    v = MulByPow2(state->LFSR[13], 17);
    f = AddM(f, v);
    v = MulByPow2(state->LFSR[15], 15);
    f = AddM(f, v);
    f = AddM(f, u);

    // 移位更新
    for (int i = 0; i < 15; i++) {
        state->LFSR[i] = state->LFSR[i + 1];
    }
    state->LFSR[15] = f;
}

// LFSR工作模式更新
static void LFSRWithWorkMode(zuc_state_t* state) {
    uint32_t f = state->LFSR[0];
    uint32_t v = MulByPow2(state->LFSR[0], 8);
    f = AddM(f, v);
    v = MulByPow2(state->LFSR[4], 20);
    f = AddM(f, v);
    v = MulByPow2(state->LFSR[10], 21);
    f = AddM(f, v);
    v = MulByPow2(state->LFSR[13], 17);
    f = AddM(f, v);
    v = MulByPow2(state->LFSR[15], 15);
    f = AddM(f, v);

    // 移位更新
    for (int i = 0; i < 15; i++) {
        state->LFSR[i] = state->LFSR[i + 1];
    }
    state->LFSR[15] = f;
}

// F函数 - 与Java版本F()一致
static uint32_t F_function(zuc_state_t* state) {
    uint32_t W, W1, W2, u, v;

    // 与Java版本完全一致的计算
    W = (state->X0 ^ state->R1) + state->R2;
    W1 = state->R1 + state->X1;
    W2 = state->R2 ^ state->X2;

    u = L1((W1 << 16) | (W2 >> 16));
    v = L2((W2 << 16) | (W1 >> 16));

    // 更新R1和R2状态
    state->R1 = MAKEU32(S0[u >> 24], S1[(u >> 16) & 0xFF],
                        S0[(u >> 8) & 0xFF], S1[u & 0xFF]);
    state->R2 = MAKEU32(S0[v >> 24], S1[(v >> 16) & 0xFF],
                        S0[(v >> 8) & 0xFF], S1[v & 0xFF]);

    return W;
}

// ZUC初始化 - 与Java版本setKeyAndIV一致
static void zuc_init(zuc_state_t* state, const uint8_t* key, const uint8_t* iv) {
    // 初始化LFSR - 与Java版本完全一致
    for (int i = 0; i < 16; i++) {
        state->LFSR[i] = MAKEU31(key[i], EK_d[i], iv[i]);
    }

    // 初始化R1和R2为0
    state->R1 = 0;
    state->R2 = 0;

    // 初始化模式运行32轮 - 与Java版本一致
    for (int i = 0; i < 32; i++) {
        BitReorganization(state);
        uint32_t w = F_function(state);
        LFSRWithInitialisationMode(state, w >> 1);
    }

    // 工作模式准备 - 与Java版本一致
    BitReorganization(state);
    F_function(state); // 丢弃输出
    LFSRWithWorkMode(state);
}

// 生成密钥流字 - 与Java版本makeKeyStreamWord一致
static uint32_t zuc_generate(zuc_state_t* state) {
    BitReorganization(state);
    uint32_t result = F_function(state) ^ state->X3;
    LFSRWithWorkMode(state);
    return result;
}

// ZUC处理数据 - 与Java版本processBytes一致
static void zuc_process_bytes(const uint8_t* key, const uint8_t* iv,
                              const uint8_t* input, uint8_t* output, size_t len) {
    zuc_state_t state;
    zuc_init(&state, key, iv);

    // 按字节处理，与Java版本returnByte一致
    uint8_t keystream[4];
    int keystream_index = 0;

    for (size_t i = 0; i < len; i++) {
        if (keystream_index == 0) {
            // 生成新的密钥流字
            uint32_t word = zuc_generate(&state);
            keystream[0] = (word >> 24) & 0xFF;
            keystream[1] = (word >> 16) & 0xFF;
            keystream[2] = (word >> 8) & 0xFF;
            keystream[3] = word & 0xFF;
        }

        output[i] = input[i] ^ keystream[keystream_index];
        keystream_index = (keystream_index + 1) % 4;
    }
}

// 加密实现 - 对应Kotlin: override fun encrypt(text: String): String
static char* zuc_encrypt(cipher_interface_t* self, const char* text) {
    if (!self || !text) return NULL;
    
    zuc_data_t* data = (zuc_data_t*)self->private_data;
    if (!data) return NULL;
    
    size_t text_len = strlen(text);
    
    // 4字节对齐填充（对应Kotlin中的4字节对齐）
    size_t padded_len;
    uint8_t* padded_data = pad_to_multiple((const uint8_t*)text, text_len, 4, &padded_len);
    if (!padded_data) return NULL;
    
    // 分配输出缓冲区
    uint8_t* output = safe_malloc(padded_len);
    
    // 执行ZUC加密
    zuc_process_bytes(data->key, data->iv, padded_data, output, padded_len);
    
    safe_free(padded_data);
    
    // 转换为大写十六进制字符串
    char* hex_result = bytes_to_hex_upper(output, padded_len);
    safe_free(output);
    
    return hex_result;
}

// 解密实现 - 对应Kotlin: override fun decrypt(hex: String): String
static char* zuc_decrypt(cipher_interface_t* self, const char* hex) {
    if (!self || !hex) return NULL;
    
    zuc_data_t* data = (zuc_data_t*)self->private_data;
    if (!data) return NULL;
    
    // 将十六进制字符串转换为字节数组
    size_t bytes_len;
    uint8_t* bytes = hex_to_bytes(hex, &bytes_len);
    if (!bytes) return NULL;
    
    // 分配输出缓冲区
    uint8_t* output = safe_malloc(bytes_len);
    
    // 执行ZUC解密（ZUC是流密码，加密和解密操作相同）
    zuc_process_bytes(data->key, data->iv, bytes, output, bytes_len);
    
    safe_free(bytes);
    
    // 移除尾部的零字节填充（对应Kotlin中的dropLastWhile）
    while (bytes_len > 0 && output[bytes_len - 1] == 0) {
        bytes_len--;
    }
    
    // 转换为字符串
    char* result = safe_malloc(bytes_len + 1);
    memcpy(result, output, bytes_len);
    result[bytes_len] = '\0';
    safe_free(output);
    
    return result;
}

// 销毁函数
static void zuc_destroy(cipher_interface_t* self) {
    if (self) {
        safe_free(self->private_data);
        safe_free(self);
    }
}

// 创建ZUC加解密实例
cipher_interface_t* create_zuc_cipher(const uint8_t* key, const uint8_t* iv) {
    if (!key || !iv) return NULL;
    
    cipher_interface_t* cipher = safe_malloc(sizeof(cipher_interface_t));
    zuc_data_t* data = safe_malloc(sizeof(zuc_data_t));
    
    // 复制密钥和IV
    memcpy(data->key, key, 16);
    memcpy(data->iv, iv, 16);
    
    // 设置函数指针
    cipher->encrypt = zuc_encrypt;
    cipher->decrypt = zuc_decrypt;
    cipher->destroy = zuc_destroy;
    cipher->private_data = data;
    
    return cipher;
}