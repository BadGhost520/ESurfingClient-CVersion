#ifndef CIPHER_UTILS_H
#define CIPHER_UTILS_H

#include <stdint.h>

#ifdef WIN32
#else
#include <stddef.h>
#endif

char* bytes_2_hex(const uint8_t* bytes, size_t len);
uint8_t* hex_2_bytes(const char* hex, size_t* out_len);
void* s_malloc(size_t size);
void* s_calloc(size_t count, size_t size);
void s_free(void* ptr);
size_t s_strlen(const char* str);
uint8_t* pkcs7_padding(const uint8_t* data, size_t data_len, size_t block_size, size_t* out_len);
uint8_t* rm_pkcs7_padding(const uint8_t* data, size_t data_len, size_t* out_len);
uint8_t* pad_2_multiple(const uint8_t* data, size_t data_len, size_t multiple, size_t* out_len);
uint32_t bytes_2_uint32_be(const uint8_t* bytes);
uint32_t bytes_2_uint32_le(const uint8_t* bytes);
void uint32_2_bytes_be(uint32_t value, uint8_t* bytes);
void uint32_2_bytes_le(uint32_t value, uint8_t* bytes);
void xor_bytes(const uint8_t* a, const uint8_t* b, uint8_t* result, size_t len);

#endif // CIPHER_UTILS_H