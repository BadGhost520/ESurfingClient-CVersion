//
// Created by bad_g on 2025/9/25.
//
#include <stdlib.h>
#include <string.h>

#include "../headFiles/utils/ByteArray.h"

#include <ctype.h>
#include <stdio.h>

ByteArray stringToBytes(const char* str)
{
    ByteArray ba = {0};
    if (!str) return ba;
    ba.length = strlen(str);
    ba.data = (unsigned char*)malloc(ba.length);
    if (ba.data)
    {
        memcpy(ba.data, str, ba.length);
    }
    return ba;
}

void printByteArray(const ByteArray* ba, const char* name)
{
    if (!ba || !ba->data)
    {
        printf("%s: NULL or empty data\n", name);
        return;
    }
    printf("=== %s (length: %zu bytes) ===\n", name, ba->length);
    printf("hexadecimal: ");
    for (size_t i = 0; i < ba->length; i++)
    {
        printf("%02X ", ba->data[i]);
        if ((i + 1) % 16 == 0) printf("\n         ");
    }
    printf("\n");
    printf("decimal:   ");
    for (size_t i = 0; i < (ba->length < 20 ? ba->length : 20); i++)
    {
        printf("%3d ", ba->data[i]);
    }
    if (ba->length > 20) printf("...");
    printf("\n");
    printf("Character display: ");
    for (size_t i = 0; i < ba->length; i++)
    {
        if (isprint(ba->data[i]))
        {
            printf("%c ", ba->data[i]);
        }
        else
        {
            printf(". ");
        }
    }
    printf("\n");
    printf("As a string: \"");
    for (size_t i = 0; i < ba->length; i++)
    {
        if (ba->data[i] == '\0')
        {
            printf("\\0");
        }
        else if (isprint(ba->data[i]))
        {
            printf("%c", ba->data[i]);
        }
        else
        {
            printf("\\x%02X", ba->data[i]);
        }
    }
    printf("\"\n\n");
}
