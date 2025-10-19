//
// Created by bad_g on 2025/9/22.
//

#ifndef ESURFINGCLIENT_CONSTANTS_H
#define ESURFINGCLIENT_CONSTANTS_H

extern const char* USER_AGENT;
extern const char* REQUEST_ACCEPT;
extern const char* CAPTIVE_URL;
extern const char* PORTAL_END_TAG;
extern const char* PORTAL_START_TAG;
extern const char* AUTH_KEY;
extern const char* HOST_NAME;

/**
 * 初始化常量函数
 */
void initConstants();

/**
 * 初始化 UA 函数
 * @param channel 通道
 */
void initChannel(int channel);

#endif //ESURFINGCLIENT_CONSTANTS_H