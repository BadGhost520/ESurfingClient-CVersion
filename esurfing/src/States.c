//
// Created by bad_g on 2025/9/22.
//
#include <string.h>
#include <stdlib.h>

#include "headFiles/utils/PlatformUtils.h"
#include "headFiles/States.h"

#include "headFiles/utils/Logger.h"

char* clientId;
char* algoId;
char* macAddress;
char* ticket;
char* userIp;
char* acIp;

volatile sig_atomic_t isRunning = 0;

char* schoolId;
char* domain;
char* area;
char* ticketUrl;
char* authUrl;
volatile sig_atomic_t isLogged = 0;

void refreshStates()
{
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
    LOG_DEBUG("Client Id: %s", clientId);
    LOG_DEBUG("MAC: %s", macAddress);
}