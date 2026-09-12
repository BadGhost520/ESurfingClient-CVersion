#ifndef SIMSSL_H
#define SIMSSL_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 与 OpenSSL 头文件互斥：本头文件是 <openssl/evp.h> 的替代品，不应与它同时包含。
 * （libcurl 仍然链接 libcrypto 时，链接层面也不会冲突，见下面的名称重映射。）
 */
#if defined(OPENSSL_EVP_H) || defined(HEADER_EVP_H) || defined(MYSSL_OPENSSL_EVP_INCLUDED)
#error "simssl/evp.h 不能与 OpenSSL 的 <openssl/evp.h> 出现在同一个编译单元中"
#endif
#define MYSSL_OPENSSL_EVP_INCLUDED 1

#define EVP_MAX_MD_SIZE            MYSSL_EVP_MAX_MD_SIZE
#define EVP_MD_CTX                 MYSSL_MD_CTX
#define EVP_MD                     MYSSL_MD
#define EVP_CIPHER_CTX             MYSSL_CIPHER_CTX
#define EVP_CIPHER                 MYSSL_CIPHER

#define EVP_MD_CTX_new             myssl_EVP_MD_CTX_new
#define EVP_MD_CTX_free            myssl_EVP_MD_CTX_free
#define EVP_md5                    myssl_EVP_md5
#define EVP_DigestInit_ex          myssl_EVP_DigestInit_ex
#define EVP_DigestUpdate           myssl_EVP_DigestUpdate
#define EVP_DigestFinal_ex         myssl_EVP_DigestFinal_ex

#define EVP_CIPHER_CTX_new         myssl_EVP_CIPHER_CTX_new
#define EVP_CIPHER_CTX_free        myssl_EVP_CIPHER_CTX_free
#define EVP_CIPHER_CTX_set_padding myssl_EVP_CIPHER_CTX_set_padding
#define EVP_aes_128_ecb            myssl_EVP_aes_128_ecb
#define EVP_aes_128_cbc            myssl_EVP_aes_128_cbc
#define EVP_des_ede3_ecb           myssl_EVP_des_ede3_ecb
#define EVP_des_ede3_cbc           myssl_EVP_des_ede3_cbc
#define EVP_EncryptInit_ex         myssl_EVP_EncryptInit_ex
#define EVP_DecryptInit_ex         myssl_EVP_DecryptInit_ex
#define EVP_EncryptUpdate          myssl_EVP_EncryptUpdate
#define EVP_DecryptUpdate          myssl_EVP_DecryptUpdate
#define EVP_EncryptFinal_ex        myssl_EVP_EncryptFinal_ex
#define EVP_DecryptFinal_ex        myssl_EVP_DecryptFinal_ex

#define MYSSL_EVP_MAX_MD_SIZE 64

typedef struct myssl_md_ctx_st EVP_MD_CTX;
typedef struct myssl_md_st EVP_MD;
typedef struct myssl_cipher_ctx_st EVP_CIPHER_CTX;
typedef struct myssl_cipher_st EVP_CIPHER;

/* ------------------------- 摘要 ------------------------- */

EVP_MD_CTX* EVP_MD_CTX_new(void);
void EVP_MD_CTX_free(EVP_MD_CTX* ctx);
const EVP_MD* EVP_md5(void);
int EVP_DigestInit_ex(EVP_MD_CTX* ctx, const EVP_MD* type, void* impl);
int EVP_DigestUpdate(EVP_MD_CTX* ctx, const void* data, size_t len);
int EVP_DigestFinal_ex(EVP_MD_CTX* ctx, unsigned char* md, unsigned int* size);

/* ------------------------ 对称密码 ------------------------ */

EVP_CIPHER_CTX* EVP_CIPHER_CTX_new(void);
void EVP_CIPHER_CTX_free(EVP_CIPHER_CTX* ctx);

const EVP_CIPHER* EVP_aes_128_ecb(void);
const EVP_CIPHER* EVP_aes_128_cbc(void);
const EVP_CIPHER* EVP_des_ede3_ecb(void);
const EVP_CIPHER* EVP_des_ede3_cbc(void);

int EVP_EncryptInit_ex(EVP_CIPHER_CTX* ctx, const EVP_CIPHER* type, void* impl,
                       const unsigned char* key, const unsigned char* iv);
int EVP_DecryptInit_ex(EVP_CIPHER_CTX* ctx, const EVP_CIPHER* type, void* impl,
                       const unsigned char* key, const unsigned char* iv);
int EVP_CIPHER_CTX_set_padding(EVP_CIPHER_CTX* ctx, int padding);

int EVP_EncryptUpdate(EVP_CIPHER_CTX* ctx, unsigned char* out, int* outl,
                      const unsigned char* in, int inl);
int EVP_DecryptUpdate(EVP_CIPHER_CTX* ctx, unsigned char* out, int* outl,
                      const unsigned char* in, int inl);
int EVP_EncryptFinal_ex(EVP_CIPHER_CTX* ctx, unsigned char* out, int* outl);
int EVP_DecryptFinal_ex(EVP_CIPHER_CTX* ctx, unsigned char* out, int* outl);

/* -------------- 底层单块接口（便于直接测试/复用） -------------- */

void simssl_md5(const unsigned char* data, size_t len, unsigned char out[16]);

void simssl_aes128_encrypt_block(const unsigned char key[16], const unsigned char in[16],
                                unsigned char out[16]);
void simssl_aes128_decrypt_block(const unsigned char key[16], const unsigned char in[16],
                                unsigned char out[16]);

void simssl_des3_encrypt_block(const unsigned char key[24], const unsigned char in[8],
                              unsigned char out[8]);
void simssl_des3_decrypt_block(const unsigned char key[24], const unsigned char in[8],
                              unsigned char out[8]);

#ifdef __cplusplus
}
#endif

#endif /* SIMSSL_H */
