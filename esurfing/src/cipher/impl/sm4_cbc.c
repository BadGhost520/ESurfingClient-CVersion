#include "../../headFiles/cipher/cipher_interface.h"
#include "../../headFiles/cipher/cipher_utils.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/**
 * SM4-CBC（PKCS7）实现：与 Kotlin SM4CBC 行为一致
 * - 加密：PKCS7 填充 → XOR IV/前一密文 → SM4 单块加密 → 输出 HEX
 * - 解密：逐块解密 → XOR IV/前一密文 → 去 PKCS7 → 输出明文
 */

#define SM4_BLOCK_SIZE 16
#define SM4_KEY_SIZE 16

typedef struct {
    uint8_t key[SM4_KEY_SIZE];
    uint8_t iv[SM4_BLOCK_SIZE];
} sm4_cbc_data_t;

// S-box
static const uint8_t SM4_SBOX[256] = {
    0xd6,0x90,0xe9,0xfe,0xcc,0xe1,0x3d,0xb7,0x16,0xb6,0x14,0xc2,0x28,0xfb,0x2c,0x05,
    0x2b,0x67,0x9a,0x76,0x2a,0xbe,0x04,0xc3,0xaa,0x44,0x13,0x26,0x49,0x86,0x06,0x99,
    0x9c,0x42,0x50,0xf4,0x91,0xef,0x98,0x7a,0x33,0x54,0x0b,0x43,0xed,0xcf,0xac,0x62,
    0xe4,0xb3,0x1c,0xa9,0xc9,0x08,0xe8,0x95,0x80,0xdf,0x94,0xfa,0x75,0x8f,0x3f,0xa6,
    0x47,0x07,0xa7,0xfc,0xf3,0x73,0x17,0xba,0x83,0x59,0x3c,0x19,0xe6,0x85,0x4f,0xa8,
    0x68,0x6b,0x81,0xb2,0x71,0x64,0xda,0x8b,0xf8,0xeb,0x0f,0x4b,0x70,0x56,0x9d,0x35,
    0x1e,0x24,0x0e,0x5e,0x63,0x58,0xd1,0xa2,0x25,0x22,0x7c,0x3b,0x01,0x21,0x78,0x87,
    0xd4,0x00,0x46,0x57,0x9f,0xd3,0x27,0x52,0x4c,0x36,0x02,0xe7,0xa0,0xc4,0xc8,0x9e,
    0xea,0xbf,0x8a,0xd2,0x40,0xc7,0x38,0xb5,0xa3,0xf7,0xf2,0xce,0xf9,0x61,0x15,0xa1,
    0xe0,0xae,0x5d,0xa4,0x9b,0x34,0x1a,0x55,0xad,0x93,0x32,0x30,0xf5,0x8c,0xb1,0xe3,
    0x1d,0xf6,0xe2,0x2e,0x82,0x66,0xca,0x60,0xc0,0x29,0x23,0xab,0x0d,0x53,0x4e,0x6f,
    0xd5,0xdb,0x37,0x45,0xde,0xfd,0x8e,0x2f,0x03,0xff,0x6a,0x72,0x6d,0x6c,0x5b,0x51,
    0x8d,0x1b,0xaf,0x92,0xbb,0xdd,0xbc,0x7f,0x11,0xd9,0x5c,0x41,0x1f,0x10,0x5a,0xd8,
    0x0a,0xc1,0x31,0x88,0xa5,0xcd,0x7b,0xbd,0x2d,0x74,0xd0,0x12,0xb8,0xe5,0xb4,0xb0,
    0x89,0x69,0x97,0x4a,0x0c,0x96,0x77,0x7e,0x65,0xb9,0xf1,0x09,0xc5,0x6e,0xc6,0x84,
    0x18,0xf0,0x7d,0xec,0x3a,0xdc,0x4d,0x20,0x79,0xee,0x5f,0x3e,0xd7,0xcb,0x39,0x48
};

static const uint32_t SM4_CK[32] = {
    0x00070e15,0x1c232a31,0x383f464d,0x545b6269,0x70777e85,0x8c939aa1,0xa8afb6bd,0xc4cbd2d9,
    0xe0e7eef5,0xfc030a11,0x181f262d,0x343b4249,0x50575e65,0x6c737a81,0x888f969d,0xa4abb2b9,
    0xc0c7ced5,0xdce3eaf1,0xf8ff060d,0x141b2229,0x30373e45,0x4c535a61,0x686f767d,0x848b9299,
    0xa0a7aeb5,0xbcc3cad1,0xd8dfe6ed,0xf4fb0209,0x10171e25,0x2c333a41,0x484f565d,0x646b7279
};
static const uint32_t SM4_FK[4] = { 0xa3b1bac6,0x56aa3350,0x677d9197,0xb27022dc };

static inline uint32_t ROL32(uint32_t x, int n){ return (x<<n) | (x>>(32-n)); }
static inline uint32_t tau(uint32_t A){
    return (SM4_SBOX[(A>>24)&0xFF]<<24) | (SM4_SBOX[(A>>16)&0xFF]<<16) |
           (SM4_SBOX[(A>>8)&0xFF]<<8) | (SM4_SBOX[A&0xFF]);
}
static inline uint32_t L(uint32_t B){ return B ^ ROL32(B,2) ^ ROL32(B,10) ^ ROL32(B,18) ^ ROL32(B,24); }
static inline uint32_t Lp(uint32_t B){ return B ^ ROL32(B,13) ^ ROL32(B,23); }
static inline uint32_t T(uint32_t X){ return L(tau(X)); }
static inline uint32_t Tp(uint32_t X){ return Lp(tau(X)); }

static void sm4_key_expansion(const uint8_t* key, uint32_t* rk){
    uint32_t MK[4];
    uint32_t K[36];
    for(int i=0;i<4;i++){
        MK[i] = ((uint32_t)key[i*4]<<24) | ((uint32_t)key[i*4+1]<<16) |
                ((uint32_t)key[i*4+2]<<8) | (uint32_t)key[i*4+3];
    }
    for(int i=0;i<4;i++){ K[i] = MK[i] ^ SM4_FK[i]; }
    for(int i=0;i<32;i++){
        K[i+4] = K[i] ^ Tp(K[i+1] ^ K[i+2] ^ K[i+3] ^ SM4_CK[i]);
        rk[i] = K[i+4];
    }
}

static void sm4_encrypt_block(const uint8_t* in, uint8_t* out, const uint32_t* rk){
    uint32_t X[36];
    for(int i=0;i<4;i++){
        X[i] = ((uint32_t)in[i*4]<<24) | ((uint32_t)in[i*4+1]<<16) |
               ((uint32_t)in[i*4+2]<<8) | (uint32_t)in[i*4+3];
    }
    for(int i=0;i<32;i++){
        X[i+4] = X[i] ^ T(X[i+1] ^ X[i+2] ^ X[i+3] ^ rk[i]);
    }
    for(int i=0;i<4;i++){
        uint32_t Y = X[35 - i];
        out[i*4]   = (uint8_t)(Y>>24);
        out[i*4+1] = (uint8_t)(Y>>16);
        out[i*4+2] = (uint8_t)(Y>>8);
        out[i*4+3] = (uint8_t)(Y);
    }
}

static void sm4_decrypt_block(const uint8_t* in, uint8_t* out, const uint32_t* rk){
    uint32_t X[36];
    for(int i=0;i<4;i++){
        X[i] = ((uint32_t)in[i*4]<<24) | ((uint32_t)in[i*4+1]<<16) |
               ((uint32_t)in[i*4+2]<<8) | (uint32_t)in[i*4+3];
    }
    for(int i=0;i<32;i++){
        X[i+4] = X[i] ^ T(X[i+1] ^ X[i+2] ^ X[i+3] ^ rk[31 - i]);
    }
    for(int i=0;i<4;i++){
        uint32_t Y = X[35 - i];
        out[i*4]   = (uint8_t)(Y>>24);
        out[i*4+1] = (uint8_t)(Y>>16);
        out[i*4+2] = (uint8_t)(Y>>8);
        out[i*4+3] = (uint8_t)(Y);
    }
}

static uint8_t* sm4_encrypt_cbc(const uint8_t* key, const uint8_t* iv, const uint8_t* plaintext,
                                size_t plaintext_len, size_t* output_len){
    size_t padded_len=0; uint8_t* padded = pkcs7_padding(plaintext, plaintext_len, SM4_BLOCK_SIZE, &padded_len);
    if(!padded) return NULL;
    uint8_t* out = safe_malloc(padded_len);
    uint8_t prev[SM4_BLOCK_SIZE]; memcpy(prev, iv, SM4_BLOCK_SIZE);
    uint32_t rk[32]; sm4_key_expansion(key, rk);
    size_t blocks = padded_len / SM4_BLOCK_SIZE;
    for(size_t i=0;i<blocks;i++){
        uint8_t xored[SM4_BLOCK_SIZE];
        xor_bytes(padded + i*SM4_BLOCK_SIZE, prev, xored, SM4_BLOCK_SIZE);
        sm4_encrypt_block(xored, out + i*SM4_BLOCK_SIZE, rk);
        memcpy(prev, out + i*SM4_BLOCK_SIZE, SM4_BLOCK_SIZE);
    }
    safe_free(padded);
    *output_len = padded_len;
    return out;
}

static uint8_t* sm4_decrypt_cbc(const uint8_t* key, const uint8_t* iv, const uint8_t* ciphertext,
                                size_t ciphertext_len, size_t* output_len){
    if(ciphertext_len % SM4_BLOCK_SIZE != 0) return NULL;
    uint8_t* out_blocks = safe_malloc(ciphertext_len);
    uint8_t prev[SM4_BLOCK_SIZE]; memcpy(prev, iv, SM4_BLOCK_SIZE);
    uint32_t rk[32]; sm4_key_expansion(key, rk);
    size_t blocks = ciphertext_len / SM4_BLOCK_SIZE;
    for(size_t i=0;i<blocks;i++){
        uint8_t dec[SM4_BLOCK_SIZE];
        const uint8_t* block = ciphertext + i*SM4_BLOCK_SIZE;
        sm4_decrypt_block(block, dec, rk);
        xor_bytes(dec, prev, out_blocks + i*SM4_BLOCK_SIZE, SM4_BLOCK_SIZE);
        memcpy(prev, block, SM4_BLOCK_SIZE);
    }
    size_t unpadded_len=0; uint8_t* unpadded = remove_pkcs7_padding(out_blocks, ciphertext_len, &unpadded_len);
    safe_free(out_blocks);
    *output_len = unpadded_len;
    return unpadded;
}

static char* sm4_cbc_encrypt(cipher_interface_t* self, const char* text){
    if(!self || !text) return NULL;
    sm4_cbc_data_t* d = (sm4_cbc_data_t*)self->private_data;
    size_t text_len = strlen(text);
    size_t out_len=0; uint8_t* out = sm4_encrypt_cbc(d->key, d->iv, (const uint8_t*)text, text_len, &out_len);
    if(!out) return NULL;
    char* hex = bytes_to_hex_upper(out, out_len);
    safe_free(out);
    return hex;
}

static char* sm4_cbc_decrypt(cipher_interface_t* self, const char* hex){
    if(!self || !hex) return NULL;
    sm4_cbc_data_t* d = (sm4_cbc_data_t*)self->private_data;
    size_t bytes_len=0; uint8_t* bytes = hex_to_bytes(hex, &bytes_len);
    if(!bytes) return NULL;
    size_t out_len=0; uint8_t* out = sm4_decrypt_cbc(d->key, d->iv, bytes, bytes_len, &out_len);
    safe_free(bytes);
    if(!out) return NULL;
    char* result = safe_malloc(out_len + 1);
    memcpy(result, out, out_len);
    result[out_len] = '\0';
    safe_free(out);
    return result;
}

static void sm4_cbc_destroy(cipher_interface_t* self){
    if(self){ safe_free(self->private_data); safe_free(self);}
}

cipher_interface_t* create_sm4_cbc_cipher(const uint8_t* key, const uint8_t* iv){
    if(!key || !iv) return NULL;
    cipher_interface_t* ci = safe_malloc(sizeof(cipher_interface_t));
    sm4_cbc_data_t* d = safe_malloc(sizeof(sm4_cbc_data_t));
    memcpy(d->key, key, SM4_KEY_SIZE);
    memcpy(d->iv, iv, SM4_BLOCK_SIZE);
    ci->encrypt = sm4_cbc_encrypt;
    ci->decrypt = sm4_cbc_decrypt;
    ci->destroy = sm4_cbc_destroy;
    ci->private_data = d;
    return ci;
}