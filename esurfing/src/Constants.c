//
// Created by bad_g on 2025/9/22.
//
#include <stdlib.h>
#include <time.h>
#include <stdint.h>

#include "headFiles/utils/Logger.h"

#ifdef _WIN32
    #define CRT_RAND_S
    #include <process.h>
#else
    #include <unistd.h>
    #include <sys/types.h>
#endif

const char* USER_LINUX_AGENT = "CCTP/Linux64/1003";
const char* USER_ANDROID_AGENT = "CCTP/android64_vpn/2093";
const char* USER_AGENT;
const char* REQUEST_ACCEPT = "text/html,text/xml,application/xhtml+xml,application/x-javascript,*/*";
const char* CAPTIVE_URL = "http://connect.rom.miui.com/generate_204";
const char* PORTAL_END_TAG = "//config.campus.js.chinatelecom.com-->";
const char* PORTAL_START_TAG = "<!--//config.campus.js.chinatelecom.com";
const char* AUTH_KEY = "Eshore!@#";
const char* HOST_NAME;

static uint32_t xorShiftState = 0;

/**
 * 初始化随机数生成器函数
 */
static void initSecureRandom() {
    static int initialized = 0;
    if (initialized) return;
#ifdef _WIN32
    xorShiftState = (uint32_t)(time(NULL) ^ _getpid() ^ clock());
#else
    xorShiftState = (uint32_t)(time(NULL) ^ getpid() ^ clock());
#endif
    if (xorShiftState == 0) {
        xorShiftState = 1;
    }
    initialized = 1;
}

/**
 * 生成随机数函数
 */
static uint32_t secureRandom() {
    initSecureRandom();
    xorShiftState ^= xorShiftState << 13;
    xorShiftState ^= xorShiftState >> 17;
    xorShiftState ^= xorShiftState << 5;
    return xorShiftState;
}

/**
 * 生成随机MAC地址
 * @return 随机 MAC 地址
 */
char* randomMAC() {
    char* mac = malloc(18 * sizeof(char));
    if (mac == NULL) {
        return NULL;
    }
    for (int i = 0; i < 6; i++) {
        const uint32_t random_value = secureRandom();
        const uint8_t byte_value = (uint8_t)(random_value & 0xFF);
        if (i == 0) {
            sprintf(mac + (i * 3), "%02X", (byte_value & 0xFE) | 0x02);
        } else {
            sprintf(mac + (i * 3), "%02X", byte_value);
        }
        if (i < 5) {
            mac[i * 3 + 2] = ':';
        }
    }
    mac[17] = '\0';
    return mac;
}

/**
 * 初始化常量函数
 */
void initConstants()
{
    HOST_NAME = randomMAC();
    LOG_DEBUG("MAC: %s", HOST_NAME);
}

/**
 * 初始化 UA 函数
 * @param channel 通道
 */
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
        LOG_ERROR("Error device\n");
        exit(1);
    }
    LOG_DEBUG("UA: %s", USER_AGENT);
}