#ifndef CIPHER_INTERFACE_H
#define CIPHER_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cipherInterface {
    char* (*encrypt)(struct cipherInterface* self, const char* text);
    char* (*decrypt)(struct cipherInterface* self, const char* hex);
    void (*destroy)(struct cipherInterface* self);
    void* private_data;
} cipherInterfaceT;

/**
 * 销毁加解密工厂
 */
void cipherFactoryDestroy();

/**
 * 初始化加解密工厂
 * @param algo_id 算法 ID
 * @return 初始化状态
 */
int initCipher(const char* algo_id);

/**
 * 加密函数
 * @param text 需要加密的文本
 * @return 加密后文本
 */
char* sessionEncrypt(const char* text);

/**
 * 解密函数
 * @param text 需要解密的文本
 * @return 加密后文本
 */
char* sessionDecrypt(const char* text);

#ifdef __cplusplus
}
#endif

#endif // CIPHER_INTERFACE_H