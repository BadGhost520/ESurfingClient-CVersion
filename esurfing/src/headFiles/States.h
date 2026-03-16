#ifndef ESURFINGCLIENT_STATES_H
#define ESURFINGCLIENT_STATES_H

extern char* macAddress;
extern char* clientId;
extern char* algoId;
extern char* ticket;
extern char* userIp;
extern char* acIp;

extern int isInitialized;
extern int isRunning;
extern int isLogged;

extern long long authTime;

extern char* ticketUrl;
extern char* schoolId;
extern char* authUrl;
extern char* domain;
extern char* area;

/**
 * 刷新状态函数
 */
void refreshStates();

#endif //ESURFINGCLIENT_STATES_H