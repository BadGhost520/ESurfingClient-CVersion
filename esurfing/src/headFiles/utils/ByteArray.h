//
// Created by bad_g on 2025/9/25.
//

#ifndef ESURFINGCLIENT_BYTEARRAY_H
#define ESURFINGCLIENT_BYTEARRAY_H

typedef struct {
    unsigned char* data;
    size_t length;
} ByteArray;

/**
 * 文本转字节函数
 * @param str 文本数据
 * @return 字节数据
 */
ByteArray stringToBytes(const char* str);

/**
 * 打印字节函数
 * @param ba 字节数据
 * @param name 字节名
 */
void printByteArray(const ByteArray* ba, const char* name);

#endif //ESURFINGCLIENT_BYTEARRAY_H