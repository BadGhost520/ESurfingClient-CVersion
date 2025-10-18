//
// Created by bad_g on 2025/9/24.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "headFiles/utils/ByteArray.h"
#include "headFiles/Session.h"
#include "headFiles/States.h"
#include "headFiles/cipher/cipher_interface.h"
#include "headFiles/utils/Logger.h"

cipher_interface_t* cipher = NULL;

char* sessionEncrypt(const char* text)
{
    return cipher->encrypt(cipher, text);
}

char* sessionDecrypt(const char* text)
{
    return cipher->decrypt(cipher, text);
}

void sessionFree()
{
    isInitialized = 0;
}

int initCipher(const char* algo_id)
{
    if (cipher != NULL)
    {
        cipher_factory_destroy(cipher);
        cipher = NULL;
    }
    cipher = cipher_factory_create(algo_id);
    if (cipher == NULL)
    {
        return 0;
    }
    return 1;
}

int load(const ByteArray* zsm)
{
    char* key;
    char* algo_id;
    LOG_DEBUG("Received zsm data length: %zu", zsm->length);
    if (!zsm->data || zsm->length == 0)
    {
        key = NULL;
        algo_id = NULL;
        return 0;
    }
    char* str = malloc(zsm->length + 1);
    if (!str)
    {
        key = NULL;
        algo_id = NULL;
        return 0;
    }
    memcpy(str, zsm->data, zsm->length);
    str[zsm->length] = '\0';
    LOG_DEBUG("Original string: %s", str);
    LOG_DEBUG("String length: %zu", strlen(str));
    if (strlen(str) < 4 + 38)
    {
        LOG_ERROR("Insufficient string length");
        free(str);
        key = NULL;
        algo_id = NULL;
        return 0;
    }
    const size_t key_length = strlen(str) - 4 - 38;
    if (key_length <= 0)
    {
        LOG_ERROR("Key length calculation error");
        free(str);
        key = NULL;
        algo_id = NULL;
        return 0;
    }
    key = (char*)malloc(key_length + 1);
    if (key)
    {
        strncpy(key, str + 4, key_length);
        (key)[key_length] = '\0';
        LOG_DEBUG("Extracted Key: %s", key);
        LOG_DEBUG("Key length: %zu", key_length);
    }
    const size_t total_length = strlen(str);
    if (total_length >= 38)
    {
        const size_t algo_id_length = 36;
        algo_id = (char*)malloc(algo_id_length + 1);
        if (algo_id)
        {
            strncpy(algo_id, str + total_length - 37, algo_id_length);
            (algo_id)[algo_id_length] = '\0';
            LOG_DEBUG("Extracted Algo ID: %s", algo_id);
        }
    }
    else
    {
        algo_id = NULL;
        LOG_ERROR("The string length is not sufficient to extract the Algo ID");
        return 0;
    }
    free(str);
    LOG_INFO("Algo ID: %s", algo_id);
    LOG_INFO("Key: %s", key);
    LOG_DEBUG("Initializing encryptor...");
    if (!initCipher(algo_id))
    {
        LOG_ERROR("Unable to initialize encryptor");
        free(key);
        free(algo_id);
        return 0;
    }
    LOG_DEBUG("Encryptor initialization successful");
    if (algoId != NULL)
    {
        free(algoId);
    }
    algoId = malloc(strlen(algo_id) + 1);
    if (algoId == NULL)
    {
        LOG_ERROR("Unable to allocate global Algo ID memory");
        free(key);
        free(algo_id);
        return 0;
    }
    strcpy(algoId, algo_id);
    LOG_DEBUG("Global Algo ID has been updated: '%s'", algoId);
    free(key);
    free(algo_id);
    LOG_DEBUG("Session loaded successfully");
    return 1;
}

void initialize(const ByteArray* zsm)
{
    LOG_DEBUG("Initializing session");
    if (load(zsm))
    {
        isInitialized = 1;
    }
}