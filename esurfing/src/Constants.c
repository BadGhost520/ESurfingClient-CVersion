#include <stdlib.h>
#include <string.h>

#include "headFiles/utils/PlatformUtils.h"
#include "headFiles/utils/Logger.h"

const char* REQUEST_ACCEPT = "text/html,text/xml,application/xhtml+xml,application/x-javascript,*/*";
const char* CAPTIVE_URL = "http://connect.rom.miui.com/generate_204";
const char* AUTH_KEY = "Eshore!@#";
const char* USER_LINUX_AGENT = "CCTP/Linux64/1003";
const char* USER_ANDROID_AGENT = "CCTP/android64_vpn/2093";
char* USER_AGENT = "CCTP/android64_vpn/2093";
char* HOST_NAME;

void setRandomHost()
{
    char* str = malloc(18 * sizeof(char));
    if (!str)
    {
        LOG_ERROR("分配内存失败");
        return;
    }
    unsigned char strBytes[10];
    randomBytes(strBytes, 10);
    strBytes[0] = strBytes[0] & 0xFEU;
    sprintf(str, "%02x%02x%02x%02x%02x",
    strBytes[0], strBytes[1],
    strBytes[2], strBytes[3],
    strBytes[4]);
    LOG_DEBUG("HOST_NAME: %s", str);
    HOST_NAME = strdup(str);
    free(str);
}

void initConstants()
{
    if (HOST_NAME) free(HOST_NAME);
    setRandomHost();
}