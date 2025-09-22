//
// Created by bad_g on 2025/9/22.
//
#include <stdbool.h>
#include "headFiles/States.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "headFiles/PlatformUtils.h"

// 内部辅助函数声明
static unsigned int hash_function(const char* key, size_t capacity);
static HashMapNode* create_node(const char* key, const char* value);
static void free_node(HashMapNode* node);
static int resize_hashmap(HashMap* map);
static void free_bucket_chain(HashMapNode* head);

char* clientId;
char* algoId;
char* macAddress;
char* ticket;
char* userIp;
char* acIp;

bool isRunning = true;

char* schoolId;
char* domain;
char* area;
char* ticketUrl;
char* authUrl;
bool isLogged = false;

void RefreshClientState(void)
{
    // 刷新客户端状态
    if (clientId)
    {
        free(clientId);
    }
    setClientId(&clientId);

    if (algoId)
    {
        free(algoId);
    }
    algoId = strdup("00000000-0000-0000-0000-000000000000");

    macAddress = strdup(randomMacAddress());
}