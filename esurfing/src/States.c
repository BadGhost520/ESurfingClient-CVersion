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

int isRunning = 0;
int isLogged = 0;
int isInitialized = 0;

long long authTime = 0;

char* schoolId;
char* domain;
char* area;
char* ticketUrl;
char* authUrl;

void refreshStates()
{
    if (clientId) free(clientId);
    if (algoId) free(algoId);
    if (macAddress) free(macAddress);
    if (ticket) free(ticket);
    if (userIp) free(userIp);
    if (acIp) free(acIp);
    if (schoolId) free(schoolId);
    if (domain) free(domain);
    if (area) free(area);
    if (ticketUrl) free(ticketUrl);
    if (authUrl) free(authUrl);
    setClientId(&clientId);
    algoId = strdup("00000000-0000-0000-0000-000000000000");
    macAddress = strdup(randomMacAddress());
    LOG_DEBUG("Client Id: %s", clientId);
    LOG_DEBUG("MAC: %s", macAddress);
}