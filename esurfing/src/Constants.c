#include <stdlib.h>

#include "headFiles/utils/PlatformUtils.h"
#include "headFiles/utils/Logger.h"

const char* REQUEST_ACCEPT = "text/html,text/xml,application/xhtml+xml,application/x-javascript,*/*";
const char* PORTAL_START_TAG = "<!--//config.campus.js.chinatelecom.com";
const char* PORTAL_END_TAG = "//config.campus.js.chinatelecom.com-->";
const char* CAPTIVE_URL = "http://connect.rom.miui.com/generate_204";
const char* AUTH_KEY = "Eshore!@#";
const char* USER_AGENT;
char* HOST_NAME;

void initConstants()
{
    if (HOST_NAME) free(HOST_NAME);
    HOST_NAME = randomString();
    LOG_DEBUG("HOST_NAME: %s", HOST_NAME);
}