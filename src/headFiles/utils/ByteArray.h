//
// Created by bad_g on 2025/9/25.
//

#ifndef ESURFINGCLIENT_BYTEARRAY_H
#define ESURFINGCLIENT_BYTEARRAY_H

typedef struct {
    unsigned char* data;
    size_t length;
} ByteArray;

ByteArray string_to_bytes(const char* str);

void print_byte_array(const ByteArray* ba, const char* name);

#endif //ESURFINGCLIENT_BYTEARRAY_H