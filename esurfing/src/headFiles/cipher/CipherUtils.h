#ifndef CIPHER_UTILS_H
#define CIPHER_UTILS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

char* bytesToHexUpper(const uint8_t* bytes, size_t len);
uint8_t* hexToBytes(const char* hex, size_t* out_len);
void* safeMalloc(size_t size);
void* safeCalloc(size_t count, size_t size);
void safeFree(void* ptr);
size_t safeStrLen(const char* str);
uint8_t* pkcs7Padding(const uint8_t* data, size_t data_len, size_t block_size, size_t* out_len);
uint8_t* removePkcs7Padding(const uint8_t* data, size_t data_len, size_t* out_len);
uint8_t* padToMultiple(const uint8_t* data, size_t data_len, size_t multiple, size_t* out_len);
uint32_t bytesToUint32Be(const uint8_t* bytes);
uint32_t bytesToUint32Le(const uint8_t* bytes);
void uint32ToBytesBe(uint32_t value, uint8_t* bytes);
void uint32ToBytesLe(uint32_t value, uint8_t* bytes);
void xorBytes(const uint8_t* a, const uint8_t* b, uint8_t* result, size_t len);

#ifdef __cplusplus
}
#endif

#endif // CIPHER_UTILS_H