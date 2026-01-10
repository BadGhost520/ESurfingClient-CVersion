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
#include <winsock2.h>
#include <iphlpapi.h>
#include <windows.h>
#include <io.h>

#pragma comment(lib, "IPHLPAPI.lib")

#else

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <ifaddrs.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#endif

#include "../headFiles/utils/PlatformUtils.h"
#include "../headFiles/DialerClient.h"
#include "../headFiles/utils/minIni.h"
#include "../headFiles/utils/Logger.h"
#include "../headFiles/utils/cJSON.h"
#include "../headFiles/NetClient.h"
#include "../headFiles/States.h"

static const char xml_header[] =
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
    "<request>\n";

static const char xml_footer[] = "</request>\n";

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
            int i = 0, j = 0;
            while (pAdapter)
            {
                if (strstr(pAdapter->Description, "Virtual") != NULL)
                {
                    pAdapter = pAdapter->Next;
                    continue;
                }
                snprintf(adaptor[i].name, ADAPTER_NAME_LENGTH, "%s", pAdapter->Description);
                snprintf(adaptor[i].ip, IP_LENGTH, "%s", pAdapter->IpAddressList.IpAddress.String);
                if (school_network_symbol[0] != '\0')
                {
                    if (strstr(pAdapter->IpAddressList.IpAddress.String, school_network_symbol) != NULL && j < MAX_DIALER_COUNT)
                    {
                        snprintf(school_connection_status[j].ip, IP_LENGTH, "%s", pAdapter->IpAddressList.IpAddress.String);
                        LOG_VERBOSE("获取到校园网 IP: %s", school_connection_status[j].ip);
                        j++;
                    }
                }
                pAdapter = pAdapter->Next;
                i++;
            }
        }
    }
    if (pAdapterInfo) free(pAdapterInfo);
#else
    struct ifaddrs *ifaddrs_ptr, *ifa;
    if (getifaddrs(&ifaddrs_ptr) == 0)
    {
        int count = 0;
        for (ifa = ifaddrs_ptr; ifa; ifa = ifa->ifa_next)
        {
            if (!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET) continue;
            if (strcmp(ifa->ifa_name, "lo") == 0) continue;
            char ip[INET_ADDRSTRLEN];
            struct sockaddr_in *addr = (struct sockaddr_in*)ifa->ifa_addr;
            if (inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip)))
            {
                if (strstr(ifa->ifa_name, "docker") != NULL) continue;
                snprintf(adaptor[count].name, ADAPTER_NAME_LENGTH, "%s", ifa->ifa_name);
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
    for (int i = 0; i < 16; i++)
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

void sleepMilliseconds(const long long milliseconds)
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
        fprintf(stderr, "错误: 获取系统时间失败\n");
        return NULL;
    }
    struct tm local_time;
#ifdef _WIN32
    if (localtime_s(&local_time, &raw_time) != 0)
    {
        free(timeStr);
        fprintf(stderr, "错误: 时间转换失败\n");
        return NULL;
    }
#else
    if (localtime_r(&raw_time, &local_time) == NULL)
    {
        free(timeStr);
        fprintf(stderr, "错误: 时间转换失败\n");
        return NULL;
    }
#endif
    switch (format)
    {
    case CONSOLE_FORMAT:
        if (strftime(timeStr, 32, "%Y-%m-%d %H:%M:%S", &local_time) == 0)
        {
            free(timeStr);
            fprintf(stderr, "错误: 格式化时间失败\n");
            return NULL;
        }
        return timeStr;
    case FILE_FORMAT:
        if (strftime(timeStr, 32, "%Y%m%d-%H%M%S", &local_time) == 0)
        {
            free(timeStr);
            fprintf(stderr, "错误: 格式化时间失败\n");
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
            safeStr(thread_status[thread_index].dialer_context.auth_config.user_agent),
            safeStr(thread_status[thread_index].dialer_context.auth_config.client_id),
            currentTime,
            safeStr(thread_status[thread_index].dialer_context.auth_config.host_name),
            safeStr(thread_status[thread_index].dialer_context.auth_config.client_ip),
            safeStr(thread_status[thread_index].dialer_context.auth_config.mac_address),
            safeStr(thread_status[thread_index].dialer_context.auth_config.host_name),
            safeStr(thread_status[thread_index].dialer_context.auth_config.ac_ip),
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
            safeStr(thread_status[thread_index].dialer_context.auth_config.user_agent),
            safeStr(thread_status[thread_index].dialer_context.auth_config.client_id),
            safeStr(thread_status[thread_index].dialer_context.auth_config.ticket),
            currentTime,
            safeStr(thread_status[thread_index].dialer_context.options.usr),
            safeStr(thread_status[thread_index].dialer_context.options.pwd),
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
            safeStr(thread_status[thread_index].dialer_context.auth_config.user_agent),
            safeStr(thread_status[thread_index].dialer_context.auth_config.client_id),
            currentTime,
            safeStr(thread_status[thread_index].dialer_context.auth_config.host_name),
            safeStr(thread_status[thread_index].dialer_context.auth_config.client_ip),
            safeStr(thread_status[thread_index].dialer_context.auth_config.ticket),
            safeStr(thread_status[thread_index].dialer_context.auth_config.mac_address),
            safeStr(thread_status[thread_index].dialer_context.auth_config.host_name),
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

void checkThreadStatus()
{
    for (int i = 0; i < MAX_DIALER_COUNT; i++)
    {
        if (thread_status[i].thread_is_running && !thread_status[i].dialer_context.runtime_status.is_running)
        {
            LOG_INFO("线程 %d 程序已停止运行, 但线程未退出, 正在退出", i + 1);
            waitThreadStop(i);
        }
    }
}

void createThread(void*(* func)(void*), void* arg)
{
    const int index = (int)(intptr_t)arg;
    thread_status[index].thread_status = pthread_create(&thread_status[index].thread, NULL, func, arg);
    thread_status[index].thread_is_running = true;
    LOG_INFO("线程: %d 已启动", index + 1);
}

void waitThreadStop(const int index)
{
    pthread_join(thread_status[index].thread, NULL);
    thread_status[index].thread_is_running = false;
    LOG_INFO("线程: %d 已退出", index + 1);
}

void threadAutoStart()
{
    for (int i = 0; i < MAX_DIALER_COUNT; i++)
    {
        if (thread_status[i].dialer_context.options.auto_start)
        {
            LOG_INFO("线程 %d 自启中", i + 1);
            createThread(dialerApp, (void*)(intptr_t)i);
            sleepMilliseconds(6000);
        }
    }
}

const char* getThreadName()
{
    switch (thread_index)
    {
    case -1:
        return "主线程";
    case 0:
        return "线程 1";
    case 1:
        return "线程 2";
    default:
        return "unknown";
    }
}

void saveIni()
{
    LOG_INFO("正在更新配置文件");
    ini_puts("Adapter1", "usr", thread_status[0].dialer_context.options.usr, DIALER_CONFIG_FILE);
    ini_puts("Adapter1", "pwd", thread_status[0].dialer_context.options.pwd, DIALER_CONFIG_FILE);
    ini_puts("Adapter1", "chn", thread_status[0].dialer_context.options.chn, DIALER_CONFIG_FILE);
    ini_putbool("Adapter1", "auto_start", thread_status[0].dialer_context.options.auto_start, DIALER_CONFIG_FILE);
    ini_puts("Adapter2", "usr", thread_status[1].dialer_context.options.usr, DIALER_CONFIG_FILE);
    ini_puts("Adapter2", "pwd", thread_status[1].dialer_context.options.pwd, DIALER_CONFIG_FILE);
    ini_puts("Adapter2", "chn", thread_status[1].dialer_context.options.chn, DIALER_CONFIG_FILE);
    ini_putbool("Adapter2", "auto_start", thread_status[1].dialer_context.options.auto_start, DIALER_CONFIG_FILE);
    ini_putl("Logger", "logger_level", getLoggerLevel(), DIALER_CONFIG_FILE);
    LOG_INFO("更新完成");
}

void loadIni()
{
    char* time_stamp = getTime(CONSOLE_FORMAT);
    printf("[%s] [PRELOAD] [INFO] 正在加载配置文件\n", time_stamp);
    if (access(DIALER_CONFIG_FILE, F_OK) != 0)
    {
        time_stamp = getTime(CONSOLE_FORMAT);
        printf("[%s] [PRELOAD] [WARN] 配置文件不存在，正在创建默认配置文件\n", time_stamp);
        free(time_stamp);
        ini_puts("Adapter1", "usr", "未配置", DIALER_CONFIG_FILE);
        ini_puts("Adapter1", "pwd", "未配置", DIALER_CONFIG_FILE);
        ini_puts("Adapter1", "chn", "phone", DIALER_CONFIG_FILE);
        ini_putbool("Adapter1", "auto_start", false, DIALER_CONFIG_FILE);
        ini_puts("Adapter2", "usr", "未配置", DIALER_CONFIG_FILE);
        ini_puts("Adapter2", "pwd", "未配置", DIALER_CONFIG_FILE);
        ini_puts("Adapter2", "chn", "phone", DIALER_CONFIG_FILE);
        ini_putbool("Adapter2", "auto_start", false, DIALER_CONFIG_FILE);
        ini_putl("Logger", "logger_level", 4, DIALER_CONFIG_FILE);
        const Options opt = {
            .usr = "未配置",
            .pwd = "未配置",
            .chn = "phone",
        };
        for (int i = 0; i < MAX_DIALER_COUNT; i++) setOpt(opt, i);
        loggerInit(4);
        LOG_INFO("创建完成并加载默认配置文件");
        return;
    }
    free(time_stamp);
    const int logger_level = ini_getl("Logger", "logger_level", 4, DIALER_CONFIG_FILE);
    loggerInit(logger_level);
    Options opt[MAX_DIALER_COUNT];
    int len = 0;
    len = ini_gets("Adapter1", "usr", "未配置", opt[0].usr, USR_LENGTH, DIALER_CONFIG_FILE);
    if (len == 0) LOG_WARN("用户名 1 读取失败, 请检查配置");
    len = ini_gets("Adapter1", "pwd", "未配置", opt[0].pwd, PWD_LENGTH, DIALER_CONFIG_FILE);
    if (len == 0) LOG_WARN("密码 1 读取失败, 请检查配置");
    len = ini_gets("Adapter1", "chn", "phone", opt[0].chn, CHN_LENGTH, DIALER_CONFIG_FILE);
    if (len == 0) LOG_WARN("通道 1 读取失败, 使用默认通道");
    opt[0].auto_start = ini_getbool("Adapter1", "auto_start", false, DIALER_CONFIG_FILE);
    len = ini_gets("Adapter2", "usr", "未配置", opt[1].usr, USR_LENGTH, DIALER_CONFIG_FILE);
    if (len == 0) LOG_WARN("用户名 2 读取失败, 请检查配置");
    len = ini_gets("Adapter2", "pwd", "未配置", opt[1].pwd, PWD_LENGTH, DIALER_CONFIG_FILE);
    if (len == 0) LOG_WARN("密码 2 读取失败, 请检查配置");
    len = ini_gets("Adapter2", "chn", "phone", opt[1].chn, CHN_LENGTH, DIALER_CONFIG_FILE);
    if (len == 0) LOG_WARN("通道 2 读取失败, 使用默认通道");
    opt[1].auto_start = ini_getbool("Adapter2", "auto_start", false, DIALER_CONFIG_FILE);
    for (int i = 0; i < MAX_DIALER_COUNT; i++) setOpt(opt[i], i);
    LOG_INFO("配置文件加载完成");
    saveIni();
}