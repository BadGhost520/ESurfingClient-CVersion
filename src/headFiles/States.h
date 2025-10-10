//
// Created by bad_g on 2025/9/22.
//

#ifndef ESURFINGCLIENT_STATES_H
#define ESURFINGCLIENT_STATES_H

#include <stdbool.h>

    extern char* clientId;
    extern char* algoId;
    extern char* macAddress;
    extern char* ticket;
    extern char* userIp;
    extern char* acIp;

    extern int isRunning;

    extern char* schoolId;
    extern char* domain;
    extern char* area;
    extern char* ticketUrl;
    extern char* authUrl;
    extern int isLogged;

    void refreshStates(void);

#endif //ESURFINGCLIENT_STATES_H