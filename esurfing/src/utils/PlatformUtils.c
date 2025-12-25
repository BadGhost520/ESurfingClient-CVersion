#include <curl/curl.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>

#ifdef _WIN32

#include <sysinfoapi.h>
#include <wincrypt.h>
#include <windows.h>
#include <io.h>

#else

#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#endif

#include "../headFiles/utils/PlatformUtils.h"
#include "../headFiles/utils/Logger.h"
#include "../headFiles/Constants.h"
#include "../headFiles/States.h"

ByteArray stringToBytes(const char* str)
{
    ByteArray ba = {0};
    if (!str) return ba;
    ba.length = strlen(str);
    ba.data = (unsigned char*)malloc(ba.length);
    if (ba.data) memcpy(ba.data, str, ba.length);
    return ba;
}

void XmlParser(const char* xmlData, const char* tag, char** parsed)
{
    if (!xmlData || !tag) return;
    char start_tag[256];
    snprintf(start_tag, sizeof(start_tag), "<%s>", tag);
    char end_tag[256];
    snprintf(end_tag, sizeof(end_tag), "</%s>", tag);
    const char* start_pos = strstr(xmlData, start_tag);
    if (!start_pos) return;
    start_pos += strlen(start_tag);
    const char* end_pos = strstr(start_pos, end_tag);
    if (!end_pos) return;
    const size_t content_length = end_pos - start_pos;
    if (content_length <= 0) return;
    char* content = malloc(content_length + 1);
    if (content)
    {
        strncpy(content, start_pos, content_length);
        content[content_length] = '\0';
        *parsed = strdup(content);
        free(content);
    }
}

int stringToLongLong(const char* str, long long* result)
{
    if (!str || !result) return 0;
    while (isspace(*str)) str++;
    if (*str == '\0') return 0;
    char* endptr;
    errno = 0;
    const long long value = strtoll(str, &endptr, 10);
    if (errno == ERANGE) return 0;
    if (endptr == str) return 0;
    while (isspace(*endptr)) endptr++;
    if (*endptr != '\0') return 0;
    *result = value;
    return 1;
}

void longLongToString(char** string, const long long num)
{
    char* result = malloc(32);
    if (result)
    {
        snprintf(result, 32, "%lld", num);
        *string = strdup(result);
        free(result);
    }
}

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
    if (gettimeofday(&tv, NULL) != 0) return -1;
    return (int64_t)(tv.tv_sec * 1000LL + tv.tv_usec / 1000LL);
#endif
}

int randomBytes(unsigned char* buffer, const size_t length)
{
#ifdef _WIN32
    HCRYPTPROV hCryptProv;
    if (!CryptAcquireContext(&hCryptProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) return 0;
    const BOOL result = CryptGenRandom(hCryptProv, length, buffer);
    CryptReleaseContext(hCryptProv, 0);
    return result ? 1 : 0;
#else
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd == -1) return 0;
    ssize_t bytesRead = read(fd, buffer, length);
    close(fd);
    return (bytesRead == (ssize_t)length) ? 1 : 0;
#endif
}

void sleepMilliseconds(const int milliseconds)
{
    if (milliseconds <= 0) return;
#ifdef _WIN32
    Sleep(milliseconds);
#else
    usleep(milliseconds * 1000);
#endif
}

void getTime(char** timestamp)
{
    time_t rawTime;
    char* timeStr = malloc(20 * sizeof(char));
    if (timeStr)
    {
        time(&rawTime);
        const struct tm* timeInfo = localtime(&rawTime);
        strftime(timeStr, 20, "%Y-%m-%d %H:%M:%S", timeInfo);
        *timestamp = strdup(timeStr);
        free(timeStr);
    }
}

void getFileTime(char** timestamp)
{
    time_t rawTime;
    char* timeStr = malloc(20 * sizeof(char));
    if (timeStr)
    {
        time(&rawTime);
        const struct tm* timeInfo = localtime(&rawTime);
        strftime(timeStr, 20, "%Y%m%d-%H%M%S", timeInfo);
        *timestamp = strdup(timeStr);
        free(timeStr);
    }
}

void createXMLPayload(const XmlChoose choose, char** payload)
{
    char* xml = malloc(1024);
    if (xml == NULL) return;
    char* currentTime;
    getTime(&currentTime);
    if (currentTime == NULL)
    {
        free(xml);
        return;
    }
    LOG_DEBUG("XML 选择代码: %d", choose);
    switch (choose)
    {
        case GetTicket:
            snprintf(xml, 1024,
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
                dialer_adapter.auth_config.client_id ? dialer_adapter.auth_config.client_id : "",
                currentTime,
                HOST_NAME ? HOST_NAME : "",
                dialer_adapter.auth_config.user_ip ? dialer_adapter.auth_config.user_ip : "",
                dialer_adapter.auth_config.mac_address ? dialer_adapter.auth_config.mac_address : "",
                HOST_NAME ? HOST_NAME : "",
                dialer_adapter.auth_config.ac_ip ? dialer_adapter.auth_config.ac_ip : ""
            );
            break;
        case Login:
            snprintf(xml, 1024,
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
                dialer_adapter.auth_config.client_id ? dialer_adapter.auth_config.client_id : "",
                dialer_adapter.auth_config.ticket ? dialer_adapter.auth_config.ticket : "",
                currentTime,
                dialer_adapter.options.usr,
                dialer_adapter.options.pwd
            );
            break;
        case Heartbeat:
        case Term:
            snprintf(xml, 1024,
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
            dialer_adapter.auth_config.client_id ? dialer_adapter.auth_config.client_id : "",
            currentTime,
            HOST_NAME ? HOST_NAME : "",
            dialer_adapter.auth_config.user_ip ? dialer_adapter.auth_config.user_ip : "",
            dialer_adapter.auth_config.ticket ? dialer_adapter.auth_config.ticket : "",
            dialer_adapter.auth_config.mac_address ? dialer_adapter.auth_config.mac_address : "",
            HOST_NAME ? HOST_NAME : ""
        );
    }
    free(currentTime);
    *payload = strdup(xml);
    free(xml);
}

void cleanCDATA(const char* text, char** cleaned)
{
    if (!text) return;
    const char* cdataStart = "<![CDATA[";
    const char* cdataEnd = "]]>";
    const char* start = strstr(text, cdataStart);
    if (!start)
    {
        *cleaned = strdup(text);
        return;
    }
    start += strlen(cdataStart);
    const char* end = strstr(start, cdataEnd);
    if (!end)
    {
        *cleaned = strdup(text);
        return;
    }
    const size_t len = end - start;
    if (len <= 0) return;
    char* result = malloc(len + 1);
    if (result)
    {
        strncpy(result, start, len);
        result[len] = '\0';
        *cleaned = strdup(result);
        free(result);
    }
}

void createThread(void*(* func)(void*), void* arg, const int index)
{
    thread_status[index].status = pthread_create(&thread_status[index].thread, NULL, func, &arg);
    if (thread_status[index].status != 0)
    {
        LOG_ERROR("认证线程启动失败，序号 %d", index);
    }
}

void waitThreadStop(const int index)
{
    pthread_join(thread_status[index].thread, NULL);
}
