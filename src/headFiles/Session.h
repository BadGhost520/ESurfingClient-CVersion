//
// Created by bad_g on 2025/9/24.
//
#ifndef ESURFINGCLIENT_SESSION_H
#define ESURFINGCLIENT_SESSION_H

#include <stdbool.h>

typedef struct {
    unsigned char* data;
    size_t length;
} ByteArray;

void SessionInitialize(const ByteArray* zsm);

bool isInitialized(void);

#endif //ESURFINGCLIENT_SESSION_H