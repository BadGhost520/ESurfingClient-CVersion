//
// Created by bad_g on 2025/9/14.
//
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/time.h>
#include <stdint.h>
#include <errno.h>

#include "../headFiles/Constants.h"
#include "../headFiles/States.h"
#include "../headFiles/utils/PlatformUtils.h"
#include "../headFiles/Options.h"

#ifdef _WIN32
    #include <windows.h>
    #include <wincrypt.h>
    #include <sysinfoapi.h>
#else
    #include <unistd.h>
    #include <time.h>
    #include <fcntl.h>
    #include <sys/types.h>
#endif

/**
 * 将字符串转换为64位长整型
 * @param str 要转换的字符串
 * @param result 转换结果的指针
 * @return 转换是否成功 (1=成功, 0=失败)
 */
int stringToLongLong(const char* str, long long* result)
{
    if (!str || !result)
    {
        return 0;
    }
    while (isspace(*str))
    {
        str++;
    }
    if (*str == '\0')
    {
        return 0;
    }
    char* endptr;
    errno = 0;
    const long long value = strtoll(str, &endptr, 10);
    if (errno == ERANGE)
    {
        return 0;
    }
    if (endptr == str)
    {
        return 0;
    }
    while (isspace(*endptr))
    {
        endptr++;
    }
    if (*endptr != '\0')
    {
        return 0;
    }
    *result = value;
    return 1;
}

/**
 * 获取当前时间的毫秒时间戳函数
 * @return 64位时间戳
 */
int64_t currentTimeMillis()
{
#ifdef _WIN32
    FILETIME ft;
    ULARGE_INTEGER uli;
    GetSystemTimeAsFileTime(&ft);
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    return (int64_t)(uli.QuadPart / 10000LL - 11644473600000LL);
#else
    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0) {
        return -1;
    }
    return (int64_t)(tv.tv_sec * 1000LL + tv.tv_usec / 1000LL);
#endif
}

/**
 * 随机数生成函数
 * @param buffer 流
 * @param length 长度
 * @return
 */
static int secureRandomBytes(unsigned char* buffer, size_t length)
{
#ifdef _WIN32
    HCRYPTPROV hCryptProv;
    if (!CryptAcquireContext(&hCryptProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
    {
        return 0;
    }
    const BOOL result = CryptGenRandom(hCryptProv, length, buffer);
    CryptReleaseContext(hCryptProv, 0);
    return result ? 1 : 0;
#else
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd == -1)
    {
        return 0;
    }
    ssize_t bytesRead = read(fd, buffer, length);
    close(fd);
    return (bytesRead == (ssize_t)length) ? 1 : 0;
#endif
}

/**
 * 睡眠函数
 * @param milliseconds 毫秒
 */
void sleepMilliseconds(const int milliseconds)
{
    if (milliseconds <= 0) return;
#ifdef _WIN32
    Sleep(milliseconds);
#else
    usleep(milliseconds * 1000)
#endif
}

/**
 * 设置客户端 ID 函数
 * @param client_id 客户端 ID
 */
void setClientId(char** client_id)
{
    *client_id = malloc(37);
    if (*client_id)
    {
        unsigned char randomBytes[16];
        secureRandomBytes(randomBytes, 16);
        snprintf(*client_id, 37,
            "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
            randomBytes[0], randomBytes[1], randomBytes[2], randomBytes[3],
            randomBytes[4], randomBytes[5],
            randomBytes[6] & 0x0F | 0x40,
            randomBytes[7] & 0x3F | 0x80,
            randomBytes[8], randomBytes[9],
            randomBytes[10], randomBytes[11],
            randomBytes[12],randomBytes[13],
            randomBytes[14], randomBytes[15]);
        for (int i = 0; (*client_id)[i]; i++)
        {
            (*client_id)[i] = (char)tolower((unsigned char)(*client_id)[i]);
        }
    }
}

/**
 * 生成随机 MAC 地址函数
 * @return MAC 地址
 */
char* randomMacAddress()
{
    char* macStr = malloc(18 * sizeof(char));
    if (macStr == NULL)
    {
        return NULL;
    }
    unsigned char macBytes[6];
    secureRandomBytes(macBytes, 6);
    macBytes[0] = macBytes[0] & 0xFEU;
    sprintf(macStr, "%02x:%02x:%02x:%02x:%02x:%02x", macBytes[0], macBytes[1], macBytes[2], macBytes[3], macBytes[4], macBytes[5]);
    return macStr;
}

/**
 * 生成10位随机字符串函数
 * @return 10位随机字符串
 */
char* randomString()
{
    char* str = malloc(18 * sizeof(char));
    if (str == NULL)
    {
        return NULL;
    }
    unsigned char strBytes[10];
    secureRandomBytes(strBytes, 10);
    strBytes[0] = strBytes[0] & 0xFEU;
    sprintf(str, "%02x%02x%02x%02x%02x", strBytes[0], strBytes[1], strBytes[2], strBytes[3], strBytes[4]);
    return str;
}

/**
 * 获取当前时间函数
 * @return 当前时间(YY-mm-dd HH-MM-SS)
 */
char* getTime()
{
    time_t rawTime;
    char* timeStr = malloc(20 * sizeof(char));
    if (timeStr == NULL)
    {
        return NULL;
    }
    time(&rawTime);
    const struct tm* timeInfo = localtime(&rawTime);
    strftime(timeStr, 20, "%Y-%m-%d %H:%M:%S", timeInfo);
    return timeStr;
}

// 格式化XML字符串的辅助函数
void formatGetTicketXml(char* buffer, const char* timeStr)
{
    snprintf(buffer, 1024,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<request>\n"
        "    <user-agent>%s</user-agent>\n"
        "    <client-id>%s</client-id>\n"
        "    <local-time>%s</local-time>\n"
        "    <host-name>%s</host-name>\n"
        "    <ipv4>%s</ipv4>\n"
        "    <ipv6></ipv6>\n"
        "    <mac>%s</mac>\n"
        "    <ostag>%s</ostag>\n"
        "    <gwip>%s</gwip>\n"
        "</request>",
        USER_AGENT ? USER_AGENT : "",
        clientId ? clientId : "",
        timeStr,
        HOST_NAME ? HOST_NAME : "",
        userIp ? userIp : "",
        macAddress ? macAddress : "",
        HOST_NAME ? HOST_NAME : "",
        acIp ? acIp : ""
    );
}

void formatLoginXml(char* buffer, const char* timeStr)
{
    snprintf(buffer, 1024,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<request>\n"
        "    <user-agent>%s</user-agent>\n"
        "    <client-id>%s</client-id>\n"
        "    <ticket>%s</ticket>\n"
        "    <local-time>%s</local-time>\n"
        "    <userid>%s</userid>\n"
        "    <passwd>%s</passwd>\n"
        "</request>",
        USER_AGENT ? USER_AGENT : "",
        clientId ? clientId : "",
        ticket ? ticket : "",
        timeStr,
        usr,
        pwd
    );
}

void formatHeartbeatXml(char* buffer, const char* timeStr)
{
    snprintf(buffer, 1024,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<request>\n"
        "    <user-agent>%s</user-agent>\n"
        "    <client-id>%s</client-id>\n"
        "    <local-time>%s</local-time>\n"
        "    <host-name>%s</host-name>\n"
        "    <ipv4>%s</ipv4>\n"
        "    <ticket>%s</ticket>\n"
        "    <ipv6></ipv6>\n"
        "    <mac>%s</mac>\n"
        "    <ostag>%s</ostag>\n"
        "</request>",
        USER_AGENT ? USER_AGENT : "",
        clientId ? clientId : "",
        timeStr,
        HOST_NAME ? HOST_NAME : "",
        userIp ? userIp : "",
        ticket ? ticket : "",
        macAddress ? macAddress : "",
        HOST_NAME ? HOST_NAME : ""
    );
}

void formatTermXml(char* buffer, const char* timeStr)
{
    snprintf(buffer, 1024,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<request>\n"
        "    <user-agent>%s</user-agent>\n"
        "    <client-id>%s</client-id>\n"
        "    <local-time>%s</local-time>\n"
        "    <host-name>%s</host-name>\n"
        "    <ipv4>%s</ipv4>\n"
        "    <ticket>%s</ticket>\n"
        "    <ipv6></ipv6>\n"
        "    <mac>%s</mac>\n"
        "    <ostag>%s</ostag>\n"
        "</request>",
        USER_AGENT ? USER_AGENT : "",
        clientId ? clientId : "",
        timeStr,
        HOST_NAME ? HOST_NAME : "",
        userIp ? userIp : "",
        ticket ? ticket : "",
        macAddress ? macAddress : "",
        HOST_NAME ? HOST_NAME : ""
    );
}

// 创建XML payload字符串
char* createXMLPayload(const char* choose)
{
    char* payload = malloc(1024);
    if (payload == NULL)
    {
        return NULL;
    }
    char* currentTime = getTime();
    if (currentTime == NULL)
    {
        free(payload);
        return NULL;
    }
    if (!strcmp(choose, "getTicket"))
    {
        formatGetTicketXml(payload, currentTime);
    }
    else if (!strcmp(choose, "login"))
    {
        formatLoginXml(payload, currentTime);
    }
    else if (!strcmp(choose, "heartbeat"))
    {
        formatHeartbeatXml(payload, currentTime);
    }
    else if (!strcmp(choose, "term"))
    {
        formatTermXml(payload, currentTime);
    }
    free(currentTime);
    return payload;
}

char* cleanCDATA(const char* text)
{
    if (!text) return NULL;
    const char* cdataStart = "<![CDATA[";
    const char* cdataEnd = "]]>";
    const char* start = strstr(text, cdataStart);
    start += strlen(cdataStart);
    char* end = strstr(start, cdataEnd);
    if (!end)
    {
        return strdup(text);
    }
    const size_t len = end - start;
    char* result = malloc(len + 1);
    if (!result) return NULL;
    strncpy(result, start, len);
    result[len] = '\0';
    return result;
}