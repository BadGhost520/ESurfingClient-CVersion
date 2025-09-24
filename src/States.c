//
// Created by bad_g on 2025/9/22.
//
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "headFiles/PlatformUtils.h"
#include "headFiles/States.h"

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