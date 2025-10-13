//
// Created by bad_g on 2025/9/25.
//
#include <stdlib.h>
#include <string.h>

#include "../headFiles/utils/ByteArray.h"

#include <ctype.h>
#include <stdio.h>

ByteArray string_to_bytes(const char* str) {
    ByteArray ba = {0};
    if (!str) return ba;

    ba.length = strlen(str);  // 注意：这会在第一个null字节处停止
    ba.data = (unsigned char*)malloc(ba.length);
    if (ba.data) {
        memcpy(ba.data, str, ba.length);
    }
    return ba;
}

void print_byte_array(const ByteArray* ba, const char* name) {
    if (!ba || !ba->data) {
        printf("%s: NULL or empty data\n", name);
        return;
    }

    printf("=== %s (length: %zu bytes) ===\n", name, ba->length);

    // 1. 十六进制格式
    printf("hexadecimal: ");
    for (size_t i = 0; i < ba->length; i++) {
        printf("%02X ", ba->data[i]);
        if ((i + 1) % 16 == 0) printf("\n         "); // 每16字节换行
    }
    printf("\n");

    // 2. 十进制格式
    printf("decimal:   ");
    for (size_t i = 0; i < (ba->length < 20 ? ba->length : 20); i++) {
        printf("%3d ", ba->data[i]);
    }
    if (ba->length > 20) printf("...");
    printf("\n");

    // 3. 字符格式（可打印字符）
    printf("Character display: ");
    for (size_t i = 0; i < ba->length; i++) {
        if (isprint(ba->data[i])) {
            printf("%c ", ba->data[i]);
        } else {
            printf(". "); // 不可打印字符显示为点
        }
    }
    printf("\n");

    // 4. 尝试作为字符串打印
    printf("As a string: \"");
    for (size_t i = 0; i < ba->length; i++) {
        if (ba->data[i] == '\0') {
            printf("\\0"); // 显示null字节
        } else if (isprint(ba->data[i])) {
            printf("%c", ba->data[i]);
        } else {
            printf("\\x%02X", ba->data[i]); // 转义不可打印字符
        }
    }
    printf("\"\n\n");
}
