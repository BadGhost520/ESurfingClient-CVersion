#include "utils/PlatformUtils.h"
#include "utils/Logger.h"
#include "utils/cJSON.h"
#include "NetClient.h"
#include "States.h"

#include <curl/curl.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>

#ifdef _WIN32

#include <sysinfoapi.h>
#include <iphlpapi.h>

#endif

static const char xml_header[] = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                                 "<request>\n";

static const char xml_footer[] = "</request>\n";

static Adapter* adaptor = NULL;

void getAdapters()
{
#ifdef _WIN32
    PIP_ADAPTER_INFO pAdapterInfo = NULL;
    ULONG ulOutBufLen = 0;
    if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW)
    {
        pAdapterInfo = (PIP_ADAPTER_INFO)malloc(ulOutBufLen);
        if (pAdapterInfo && GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == NO_ERROR)
        {
            PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
            uint8_t count = 0;
            while (pAdapter)
            {
                Adapter* new_adaptor = realloc(adaptor, sizeof(Adapter) * (count + 1));
                if (!new_adaptor)
                {
                    LOG_ERROR("分配内存失败");
                    break;
                }
                adaptor = new_adaptor;
                snprintf(adaptor[count].name, NAME_LENGTH, "%s", pAdapter->Description);
                snprintf(adaptor[count].ip, IP_LENGTH, "%s", pAdapter->IpAddressList.IpAddress.String);
                LOG_VERBOSE("IP: %s", pAdapter->IpAddressList.IpAddress.String);
                pAdapter = pAdapter->Next;
                count++;
            }
        }
    }
    if (pAdapterInfo) free(pAdapterInfo);
#else
    struct ifaddrs *ifaddrs_ptr, *ifa;
    if (getifaddrs(&ifaddrs_ptr) == 0)
    {
        uint8_t count = 0;
        for (ifa = ifaddrs_ptr; ifa; ifa = ifa->ifa_next)
        {
            if (!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET) continue;
            if (strcmp(ifa->ifa_name, "lo") == 0) continue;
            char ip[INET_ADDRSTRLEN];
            struct sockaddr_in *addr = (struct sockaddr_in*)ifa->ifa_addr;
            if (inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip)))
            {
                Adapter* new_adaptor = realloc(adaptor, sizeof(Adapter) * (count + 1));
                if (!new_adaptor)
                {
                    LOG_ERROR("分配内存失败");
                    break;
                }
                adaptor = new_adaptor;
                snprintf(adaptor[count].name, NAME_LENGTH, "%s", ifa->ifa_name);
                snprintf(adaptor[count].ip, IP_LENGTH, "%s", ip);
                count++;
            }
        }
        freeifaddrs(ifaddrs_ptr);
    }
#endif
}

char* getAdaptersJSON()
{
    cJSON* root = cJSON_CreateObject();
    cJSON* adapters = cJSON_CreateArray();
    for (uint8_t i = 0; i < 16; i++)
    {
        if (strlen(adaptor[i].name) == 0) break;
        cJSON* adapter = cJSON_CreateObject();
        cJSON_AddStringToObject(adapter, "name", adaptor[i].name);
        cJSON_AddStringToObject(adapter, "ip", adaptor[i].ip);
        cJSON_AddItemToArray(adapters, adapter);
    }
    cJSON_AddItemToObject(root, "adapters", adapters);
    cJSON_AddStringToObject(root, "school_network_symbol", school_network_symbol);
    char* json = cJSON_Print(root);
    cJSON_Delete(root);
    return json;
}

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

uint64_t stringToUint64(const char* str)
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

char* uint64ToString(const uint64_t num)
{
    const uint8_t buf_size = 22;
    char* result = malloc(buf_size);
    if (!result) return NULL;
    snprintf(result, buf_size, "%" PRIu64, num);
    return result;
}

uint64_t currentTimeMillis()
{
#ifdef _WIN32
    FILETIME ft;
    ULARGE_INTEGER uli;
    GetSystemTimeAsFileTime(&ft);
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    return uli.QuadPart / 10000LL - 11644473600000LL;
#else
    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0) return -1;
    return tv.tv_sec * 1000LL + tv.tv_usec / 1000LL;
#endif
}

void randomBytes(unsigned char* buffer, const size_t length)
{
#ifdef _WIN32
    HCRYPTPROV hCryptProv;
    if (!CryptAcquireContext(&hCryptProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) return;
    CryptGenRandom(hCryptProv, length, buffer);
    CryptReleaseContext(hCryptProv, 0);
#else
    uint32_t fd = open("/dev/urandom", O_RDONLY);
    if (fd == -1) return 0;
    ssize_t bytesRead = read(fd, buffer, length);
    close(fd);
#endif
}

void sleepMilliseconds(const uint64_t milliseconds)
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
    static char timeStr[32] = "";
    time_t raw_time;
    if (time(&raw_time) == (time_t)-1)
    {
        fprintf(stderr, "错误: 获取系统时间失败\n");
        return NULL;
    }
    struct tm local_time;
#ifdef _WIN32
    if (localtime_s(&local_time, &raw_time) != 0)
    {
        fprintf(stderr, "错误: 时间转换失败\n");
        return NULL;
    }
#else
    if (localtime_r(&raw_time, &local_time) == NULL)
    {
        fprintf(stderr, "错误: 时间转换失败\n");
        return NULL;
    }
#endif
    switch (format)
    {
    case CONSOLE_FORMAT:
        if (strftime(timeStr, 32, "%Y-%m-%d %H:%M:%S", &local_time) == 0)
        {
            fprintf(stderr, "错误: 格式化时间失败\n");
            return NULL;
        }
        return timeStr;
    case FILE_FORMAT:
        if (strftime(timeStr, 32, "%Y%m%d-%H%M%S", &local_time) == 0)
        {
            fprintf(stderr, "错误: 格式化时间失败\n");
            return NULL;
        }
        return timeStr;
    default:
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
    if (!currentTime)
    {
        return NULL;
    }
    static char xml[XML_BUFFER_SIZE] = "";
    LOG_DEBUG("XML 选择代码: %d", choose);
    uint16_t xml_len = 0;
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
            safeStr(prog_status[prog_index].auth_config.user_agent),
            safeStr(prog_status[prog_index].auth_config.client_id),
            currentTime,
            safeStr(prog_status[prog_index].auth_config.host_name),
            safeStr(prog_status[prog_index].auth_config.client_ip),
            safeStr(prog_status[prog_index].auth_config.mac_address),
            safeStr(prog_status[prog_index].auth_config.host_name),
            safeStr(prog_status[prog_index].auth_config.ac_ip),
            xml_footer
        );
        break;
    case LOGIN:
        xml_len = snprintf(xml, XML_BUFFER_SIZE,
            "%s"
            "    <user-agent>%s</user-agent>\n"
            "    <client-id>%s</client-id>\n"
            "    <ticket>%s</ticket>\n"
            "    <local-time>%s</local-time>\n"
            "    <userid>%s</userid>\n"
            "    <passwd>%s</passwd>\n"
            "%s",
            xml_header,
            safeStr(prog_status[prog_index].auth_config.user_agent),
            safeStr(prog_status[prog_index].auth_config.client_id),
            safeStr(prog_status[prog_index].auth_config.ticket),
            currentTime,
            safeStr(prog_status[prog_index].login_config.usr),
            safeStr(prog_status[prog_index].login_config.pwd),
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
            safeStr(prog_status[prog_index].auth_config.user_agent),
            safeStr(prog_status[prog_index].auth_config.client_id),
            currentTime,
            safeStr(prog_status[prog_index].auth_config.host_name),
            safeStr(prog_status[prog_index].auth_config.client_ip),
            safeStr(prog_status[prog_index].auth_config.ticket),
            safeStr(prog_status[prog_index].auth_config.mac_address),
            safeStr(prog_status[prog_index].auth_config.host_name),
            xml_footer
        );
        break;
    default:
        free(currentTime);
        LOG_ERROR("XML 选择代码错误");
        return NULL;
    }
    free(currentTime);
    if (xml_len <= 0)
    {
        LOG_ERROR("XML 创建失败");
        return NULL;
    }
    if (xml_len >= XML_BUFFER_SIZE)
    {
        LOG_ERROR("XML内容过长 (需要%d字节，但缓冲区只有%d字节)", xml_len + 1, XML_BUFFER_SIZE);
        return NULL;
    }
    LOG_DEBUG("创建 XML 完成");
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

void saveJSON()
{
    cJSON* root = cJSON_CreateObject();

    cJSON_AddNumberToObject(root, "logger_level", getLoggerLevel());

    cJSON* adapters = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "adapters", adapters);

    for (uint8_t i = 0; i < prog_count; i++)
    {
        cJSON* adapter = cJSON_CreateObject();

        cJSON_AddStringToObject(adapter, "username", prog_status[i].login_config.usr);
        cJSON_AddStringToObject(adapter, "password", prog_status[i].login_config.pwd);
        cJSON_AddStringToObject(adapter, "channel", prog_status[i].login_config.chn);
        cJSON_AddStringToObject(adapter, "bind_ip", prog_status[i].login_config.ip);
        cJSON_AddBoolToObject(adapter, "auto_start", prog_status[i].login_config.auto_start);

        cJSON_AddItemToArray(adapters, adapter);
    }

    char* json = cJSON_Print(root);

    FILE* config = fopen(DIALER_CONFIG_FILE, "w");
    if (!config)
    {
        LOG_ERROR("无法生成文件: %s", DIALER_CONFIG_FILE);
        return;
    }
    fprintf(config, "%s", json);
    fclose(config);

    free(json);
    cJSON_Delete(root);
}

void loadJSON()
{
    FILE* config = fopen(DIALER_CONFIG_FILE, "rb");
    if (!config)
    {
        LOG_ERROR("无法打开文件: %s", DIALER_CONFIG_FILE);
        return;
    }

    fseek(config, 0, SEEK_END);
    const long len = ftell(config);
    fseek(config, 0, SEEK_SET);

    char* config_data = malloc(len + 1);
    fread(config_data, 1, len, config);
    config_data[len] = '\0';
    fclose(config);

    cJSON* root = cJSON_Parse(config_data);
    free(config_data);
    if (!root)
    {
        LOG_ERROR("JSON 解析失败");
        return;
    }

    const cJSON* logger_level = cJSON_GetObjectItem(root, "logger_level");
    if (logger_level) LOG_INFO("logger_level = %d", logger_level->valueint);

    cJSON* adapters = cJSON_GetObjectItem(root, "Adapter");
    if (!adapters || !cJSON_IsArray(adapters))
    {
        printf("没有找到 Adapter 数组\n");
        cJSON_Delete(root);
        return;
    }

    const uint8_t count = cJSON_GetArraySize(adapters);
    prog_status = malloc(sizeof(ProgStatus) * count);
    prog_count = count;

    for (uint8_t i = 0; i < count; i++)
    {
        const cJSON* adapter = cJSON_GetArrayItem(adapters, i);

        const cJSON* usr = cJSON_GetObjectItem(adapter, "username");
        const cJSON* pwd = cJSON_GetObjectItem(adapter, "password");
        const cJSON* chn = cJSON_GetObjectItem(adapter, "channel");
        const cJSON* ip = cJSON_GetObjectItem(adapter, "bind_ip");
        const cJSON* auto_start = cJSON_GetObjectItem(adapter, "auto_start");

        snprintf(prog_status[i].login_config.usr, USR_LENGTH, "%s", usr->valuestring);
        snprintf(prog_status[i].login_config.pwd, PWD_LENGTH, "%s", pwd->valuestring);
        snprintf(prog_status[i].login_config.chn, CHN_LENGTH, "%s", chn->valuestring);
        snprintf(prog_status[i].login_config.ip, IP_LENGTH, "%s", ip->valuestring);
        prog_status[i].login_config.auto_start = auto_start->valueint;
        if (strcmp(chn->valuestring, "pc") == 0)
        {
            snprintf(prog_status[i].auth_config.user_agent, USER_AGENT_LENGTH,  "CCTP/Linux64/1003");
        }
        else
        {
            snprintf(prog_status[i].auth_config.user_agent, USER_AGENT_LENGTH, "CCTP/android64_vpn/2093");
        }
    }

    cJSON_Delete(root);
}