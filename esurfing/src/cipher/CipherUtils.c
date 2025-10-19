#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../headFiles/cipher/CipherUtils.h"

char* bytesToHexUpper(const uint8_t* bytes, const size_t len)
{
    if (!bytes || len == 0) return NULL;
    char* hex = safeMalloc(len * 2 + 1);
    if (!hex) return NULL;
    for (size_t i = 0; i < len; i++)
    {
        sprintf(hex + i * 2, "%02X", bytes[i]);
    }
    hex[len * 2] = '\0';
    return hex;
}

uint8_t* hexToBytes(const char* hex, size_t* out_len)
{
    if (!hex || !out_len) return NULL;
    const size_t hex_len = strlen(hex);
    if (hex_len % 2 != 0) return NULL;
    const size_t byte_len = hex_len / 2;
    uint8_t* bytes = safeMalloc(byte_len);
    if (!bytes) return NULL;
    for (size_t i = 0; i < byte_len; i++)
    {
        const char byte_str[3] = {hex[i * 2], hex[i * 2 + 1], '\0'};
        bytes[i] = (uint8_t)strtol(byte_str, NULL, 16);
    }
    *out_len = byte_len;
    return bytes;
}

void* safeMalloc(const size_t size)
{
    if (size == 0) return NULL;
    void* ptr = malloc(size);
    if (!ptr)
    {
        fprintf(stderr, "Memory allocation failed: %zu bytes\n", size);
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void* safeCalloc(const size_t count, const size_t size)
{
    if (count == 0 || size == 0) return NULL;
    void* ptr = calloc(count, size);
    if (!ptr)
    {
        fprintf(stderr, "Memory allocation failed: %zu * %zu bytes\n", count, size);
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void safeFree(void* ptr)
{
    if (ptr)
    {
        free(ptr);
    }
}

size_t safeStrLen(const char* str)
{
    return str ? strlen(str) : 0;
}

uint8_t* pkcs7Padding(const uint8_t* data, const size_t data_len, const size_t block_size, size_t* out_len)
{
    if (!data || !out_len || block_size == 0) return NULL;
    size_t padding = block_size - data_len % block_size;
    if (padding == 0) padding = block_size;
    const size_t padded_len = data_len + padding;
    uint8_t* padded = safeMalloc(padded_len);
    memcpy(padded, data, data_len);
    memset(padded + data_len, (uint8_t)padding, padding);
    *out_len = padded_len;
    return padded;
}

uint8_t* removePkcs7Padding(const uint8_t* data, const size_t data_len, size_t* out_len)
{
    if (!data || !out_len || data_len == 0) return NULL;
    const uint8_t padding = data[data_len - 1];
    if (padding == 0 || padding > data_len) return NULL;
    for (size_t i = data_len - padding; i < data_len; i++)
    {
        if (data[i] != padding) return NULL;
    }
    const size_t unpadded_len = data_len - padding;
    uint8_t* unpadded = safeMalloc(unpadded_len);
    memcpy(unpadded, data, unpadded_len);
    *out_len = unpadded_len;
    return unpadded;
}

uint8_t* padToMultiple(const uint8_t* data, const size_t data_len, const size_t multiple, size_t* out_len) {
    if (!data || !out_len || multiple == 0) return NULL;
    size_t padding = (multiple - data_len % multiple) % multiple;
    size_t padded_len = data_len + padding;
    uint8_t* padded = safeCalloc(padded_len, 1);
    memcpy(padded, data, data_len);
    *out_len = padded_len;
    return padded;
}

uint32_t bytesToUint32Be(const uint8_t* bytes)
{
    if (!bytes) return 0;
    return (uint32_t)bytes[0] << 24 |
           (uint32_t)bytes[1] << 16 |
           (uint32_t)bytes[2] << 8 |
           (uint32_t)bytes[3];
}

uint32_t bytesToUint32Le(const uint8_t* bytes)
{
    if (!bytes) return 0;
    return (uint32_t)bytes[3] << 24 |
           (uint32_t)bytes[2] << 16 |
           (uint32_t)bytes[1] << 8 |
           (uint32_t)bytes[0];
}

void uint32ToBytesBe(const uint32_t value, uint8_t* bytes)
{
    if (!bytes) return;
    bytes[0] = (uint8_t)(value >> 24);
    bytes[1] = (uint8_t)(value >> 16);
    bytes[2] = (uint8_t)(value >> 8);
    bytes[3] = (uint8_t)value;
}

void uint32ToBytesLe(const uint32_t value, uint8_t* bytes)
{
    if (!bytes) return;
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8);
    bytes[2] = (uint8_t)(value >> 16);
    bytes[3] = (uint8_t)(value >> 24);
}

void xorBytes(const uint8_t* a, const uint8_t* b, uint8_t* result, const size_t len)
{
    if (!a || !b || !result) return;
    for (size_t i = 0; i < len; i++)
    {
        result[i] = a[i] ^ b[i];
    }
}