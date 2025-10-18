//
// Created by bad_g on 2025/9/22.
//

#ifndef ESURFINGCLIENT_STATES_H
#define ESURFINGCLIENT_STATES_H
#include <signal.h>

extern char* clientId;
extern char* algoId;
extern char* macAddress;
extern char* ticket;
extern char* userIp;
extern char* acIp;

extern volatile sig_atomic_t isRunning;
extern volatile sig_atomic_t isLogged;
extern volatile sig_atomic_t isInitialized;

extern char* schoolId;
extern char* domain;
extern char* area;
extern char* ticketUrl;
extern char* authUrl;

/**
 * 刷新状态函数
 */
void refreshStates();

#endif //ESURFINGCLIENT_STATES_H