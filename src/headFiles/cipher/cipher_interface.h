#ifndef CIPHER_INTERFACE_H
#define CIPHER_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 加解密接口 - 对应Kotlin的CipherInterface
 * 
 * 这个接口定义了所有加解密算法必须实现的基本方法：
 * - encrypt: 加密明文字符串，返回十六进制字符串
 * - decrypt: 解密十六进制字符串，返回明文字符串
 */

typedef struct cipher_interface {
    // 加密函数指针 - 对应Kotlin: fun encrypt(text: String): String
    char* (*encrypt)(struct cipher_interface* self, const char* text);
    
    // 解密函数指针 - 对应Kotlin: fun decrypt(hex: String): String  
    char* (*decrypt)(struct cipher_interface* self, const char* hex);
    
    // 释放资源函数指针
    void (*destroy)(struct cipher_interface* self);
    
    // 私有数据指针，用于存储具体实现的数据
    void* private_data;
} cipher_interface_t;

/**
 * 加解密工厂 - 对应Kotlin的CipherFactory
 * 
 * 支持的算法ID（与Kotlin版本完全一致）：
 * - CAFBCBAD-B6E7-4CAB-8A67-14D39F00CE1E: AES-CBC
 * - A474B1C2-3DE0-4EA2-8C5F-7093409CE6C4: AES-ECB  
 * - 5BFBA864-BBA9-42DB-8EAD-49B5F412BD81: 3DES-CBC
 * - 6E0B65FF-0B5B-459C-8FCE-EC7F2BEA9FF5: 3DES-ECB
 * - B809531F-0007-4B5B-923B-4BD560398113: ZUC-128
 * - F3974434-C0DD-4C20-9E87-DDB6814A1C48: SM4-CBC
 * - ED382482-F72C-4C41-A76D-28EEA0F1F2AF: SM4-ECB
 * - B3047D4E-67DF-4864-A6A5-DF9B9E525C79: ModXTEA
 * - C32C68F9-CA81-4260-A329-BBAFD1A9CCD1: ModXTEAIV
 */

// 创建加解密实例 - 对应Kotlin: CipherFactory.getInstance(type: String)
cipher_interface_t* cipher_factory_create(const char* algorithm_id);

// 销毁加解密实例
void cipher_factory_destroy(cipher_interface_t* cipher);

#ifdef __cplusplus
}
#endif

#endif // CIPHER_INTERFACE_H