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
 * @param algoId 算法 ID
 * @return 初始化状态
 */
int initCipher(const char* algoId);

/**
 * 加密函数
 * @param text 需要加密的文本
 * @param encrypt 加密后文本的指针
 */
void sessionEncrypt(const char* text, char** encrypt);

/**
 * 解密函数
 * @param text 需要解密的文本
 * @param decrypt 解密后文本的指针
 */
void sessionDecrypt(const char* text, char** decrypt);

#ifdef __cplusplus
}
#endif

#endif // CIPHER_INTERFACE_H