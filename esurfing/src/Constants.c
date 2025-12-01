//
// Created by bad_g on 2025/9/22.
//
#include <stdlib.h>

#include "headFiles/utils/Logger.h"
#include "headFiles/utils/PlatformUtils.h"

const char* USER_LINUX_AGENT = "CCTP/Linux64/1003";
const char* USER_ANDROID_AGENT = "CCTP/android64_vpn/2093";
const char* USER_AGENT;
const char* REQUEST_ACCEPT = "text/html,text/xml,application/xhtml+xml,application/x-javascript,*/*";
const char* CAPTIVE_URL = "http://connect.rom.miui.com/generate_204";
const char* PORTAL_END_TAG = "//config.campus.js.chinatelecom.com-->";
const char* PORTAL_START_TAG = "<!--//config.campus.js.chinatelecom.com";
const char* AUTH_KEY = "Eshore!@#";
char* HOST_NAME;

void initConstants()
{
    if (HOST_NAME) free(HOST_NAME);
    HOST_NAME = randomString();
    LOG_DEBUG("HOST_NAME: %s", HOST_NAME);
}

void initChannel(const int channel)
{
    if (channel == 1)
    {
        USER_AGENT = USER_LINUX_AGENT;
    }
    else if (channel == 2)
    {
        USER_AGENT = USER_ANDROID_AGENT;
    }
    else
    {
        LOG_ERROR("通道错误\n");
        exit(1);
    }
    LOG_DEBUG("UA: %s", USER_AGENT);
}