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

char* XmlParser(const char* xmlData, const char* tag)
{
    if (!xmlData || !tag) return NULL;
    char start_tag[256];
    snprintf(start_tag, sizeof(start_tag), "<%s>", tag);
    char end_tag[256];
    snprintf(end_tag, sizeof(end_tag), "</%s>", tag);
    const char* start_pos = strstr(xmlData, start_tag);
    if (!start_pos) return NULL;
    start_pos += strlen(start_tag);
    const char* end_pos = strstr(start_pos, end_tag);
    if (!end_pos) return NULL;
    const size_t content_length = end_pos - start_pos;
    if (content_length <= 0) return NULL;
    char* content = malloc(content_length + 1);
    if (!content) return NULL;
    strncpy(content, start_pos, content_length);
    content[content_length] = '\0';
    return content;
}

long long stringToLongLong(const char* str)
{
    if (!str) return 0;
    while (isspace(*str)) str++;
    if (*str == '\0') return 0;
    char* endptr;
    errno = 0;
    const long long value = strtoll(str, &endptr, 10);
    if (errno == ERANGE) return 0;
    if (endptr == str) return 0;
    while (isspace(*endptr)) endptr++;
    if (*endptr != '\0') return 0;
    return value;
}

char* longLongToString(const long long num)
{
    const int max_digits = 20;
    const int buf_size = max_digits + 2;
    char* result = malloc(buf_size);
    if (!result) return NULL;
    snprintf(result, buf_size, "%lld", num);
    return result;
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

char* getTime(const TimeFormat format)
{
    char* timeStr = malloc(32);
    if (!timeStr) return NULL;
    time_t raw_time;
    if (time(&raw_time) == (time_t)-1)
    {
        free(timeStr);
        LOG_ERROR("获取系统时间失败");
        return NULL;
    }
    struct tm local_time;
#ifdef _WIN32
    if (localtime_s(&local_time, &raw_time) != 0)
    {
        free(timeStr);
        LOG_ERROR("时间转换失败");
        return NULL;
    }
#else
    if (localtime_r(&raw_time, &local_time) == NULL)
    {
        free(timeStr);
        LOG_ERROR("时间转换失败");
        return NULL;
    }
#endif
    LOG_DEBUG("时间格式代码: %d", format);
    switch (format)
    {
    case CONSOLE_FORMAT:
        if (strftime(timeStr, 32, "%Y-%m-%d %H:%M:%S", &local_time) == 0)
        {
            free(timeStr);
            LOG_ERROR("格式化时间失败");
            return NULL;
        }
        return timeStr;
    case FILE_FORMAT:
        if (strftime(timeStr, 32, "%Y%m%d-%H%M%S", &local_time) == 0)
        {
            free(timeStr);
            LOG_ERROR("格式化时间失败");
            return NULL;
        }
        return timeStr;
    default:
        free(timeStr);
        return NULL;
    }
}

static const char* safeStr(const char* str)
{
    return str ? str : "";
}

char* createXMLPayload(const XmlChoose choose)
{
    char* currentTime = getTime(CONSOLE_FORMAT);
    if (!currentTime) return NULL;
    char* xml = malloc(XML_BUFFER_SIZE);
    if (!xml)
    {
        free(currentTime);
        return NULL;
    }
    LOG_DEBUG("XML 选择代码: %d", choose);
    int xml_len = 0;
    switch (choose)
    {
    case GET_TICKET:
        xml_len = snprintf(xml, XML_BUFFER_SIZE,
            "%s"
            "    <user-agent>%s</user-agent>\n"
            "    <client-id>%s</client-id>\n"
            "    <local-time>%s</local-time>\n"
            "    <host-name>%s</host-name>\n"
            "    <ipv4>%s</ipv4>\n"
            "    <ipv6></ipv6>\n"
            "    <mac>%s</mac>\n"
            "    <ostag>%s</ostag>\n"
            "    <gwip>%s</gwip>\n"
            "%s",
            xml_header,
            safeStr(USER_AGENT),
            safeStr(dialer_adapter.auth_config.client_id),
            currentTime,
            safeStr(HOST_NAME),
            safeStr(dialer_adapter.auth_config.user_ip),
            safeStr(dialer_adapter.auth_config.mac_address),
            safeStr(HOST_NAME),
            safeStr(dialer_adapter.auth_config.ac_ip),
            xml_footer
        );
        break;
    case LOGIN:
        xml_len = snprintf(xml, XML_BUFFER_SIZE,
            "%s"
            "    <user-agent>%s</user-agent>\n"
            "    <client-id>%s</client-id>\n"
            "    <local-time>%s</local-time>\n"
            "    <host-name>%s</host-name>\n"
            "    <userid>%s</userid>\n"
            "    <passwd>%s</passwd>\n"
            "%s",
            xml_header,
            safeStr(USER_AGENT),
            safeStr(dialer_adapter.auth_config.client_id),
            currentTime,
            safeStr(HOST_NAME),
            safeStr(dialer_adapter.options.usr),
            safeStr(dialer_adapter.options.pwd),
            xml_footer
        );
        break;
    case HEART_BEAT:
    case TERM:
        xml_len = snprintf(xml, XML_BUFFER_SIZE,
            "%s"
            "    <user-agent>%s</user-agent>\n"
            "    <client-id>%s</client-id>\n"
            "    <local-time>%s</local-time>\n"
            "    <host-name>%s</host-name>\n"
            "    <ipv4>%s</ipv4>\n"
            "    <ticket>%s</ticket>\n"
            "    <ipv6></ipv6>\n"
            "    <mac>%s</mac>\n"
            "    <ostag>%s</ostag>\n"
            "%s",
            xml_header,
            safeStr(USER_AGENT),
            safeStr(dialer_adapter.auth_config.client_id),
            currentTime,
            safeStr(HOST_NAME),
            safeStr(dialer_adapter.auth_config.user_ip),
            safeStr(dialer_adapter.auth_config.ticket),
            safeStr(dialer_adapter.auth_config.mac_address),
            safeStr(HOST_NAME),
            xml_footer
        );
        break;
    default:
        free(xml);
        free(currentTime);
        LOG_ERROR("XML 选择代码错误");
        return NULL;
    }
    free(currentTime);
    if (xml_len <= 0)
    {
        LOG_ERROR("XML 创建失败");
        free(xml);
        return NULL;
    }
    if (xml_len >= XML_BUFFER_SIZE)
    {
        LOG_ERROR("XML内容过长（需要%d字节，但缓冲区只有%d字节）", xml_len + 1, XML_BUFFER_SIZE);
        free(xml);
        return NULL;
    }
    return xml;
}

char* extractBetweenTags(const char* text, const char* start_tag, const char* end_tag)
{
    if (!text)
    {
        LOG_ERROR("传入文本为空");
        return NULL;
    }
    char* start = strstr(text, start_tag);
    if (!start)
    {
        LOG_ERROR("未找到开头标签: %s", start_tag);
        return NULL;
    }
    start += strlen(start_tag);
    char* end = strstr(start, end_tag);
    if (!end)
    {
        LOG_WARN("未找到结尾标签: %s, 返回", end_tag);
        return NULL;
    }
    const size_t len = end - start;
    if (len == 0) LOG_WARN("提取到空内容 (标签: %s...%s)", start_tag, end_tag);
    char* result = malloc(len + 1);
    if (!result)
    {
        LOG_ERROR("分配内存失败");
        return NULL;
    }
    memcpy(result, start, len);
    result[len] = '\0';
    return result;
}

char* cleanCDATA(const char* text)
{
    return extractBetweenTags(text, "<![CDATA[", "]]>");
}

void createThread(void*(* func)(void*), void* arg)
{
    const int index = (int)(intptr_t)arg;
    thread_status[index - 1].status = pthread_create(&thread_status[index - 1].thread, NULL, func, arg);
    if (thread_status[index - 1].status != 0) LOG_ERROR("认证线程启动失败，序号 %d", index);
}

void waitThreadStop(const int index)
{
    pthread_join(thread_status[index].thread, NULL);
}
