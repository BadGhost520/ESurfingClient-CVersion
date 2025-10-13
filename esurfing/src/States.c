//
// Created by bad_g on 2025/9/22.
//
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

int isRunning = 1;

char* schoolId;
char* domain;
char* area;
char* ticketUrl;
char* authUrl;
int isLogged = 0;

void refreshStates(void)
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