//
// PlatformUtils.c - 跨平台工具函数实现
// Created by bad_g on 2025/9/14.
//

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "headFiles/Constants.h"
#include "headFiles/States.h"
#include "headFiles/PlatformUtils.h"
#include "headFiles/Options.h"

// 平台特定的头文件
#ifdef _WIN32
    #include <windows.h>
    #include <wincrypt.h>
    #include <process.h>
#else
    #include <unistd.h>
    #include <time.h>
    #include <fcntl.h>
    #include <sys/types.h>
#endif

// 安全的随机数生成函数
static int secure_random_bytes(unsigned char* buffer, size_t length) {
#ifdef _WIN32
    HCRYPTPROV hCryptProv;
    if (!CryptAcquireContext(&hCryptProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        return 0; // 失败
    }

    BOOL result = CryptGenRandom(hCryptProv, (DWORD)length, buffer);
    CryptReleaseContext(hCryptProv, 0);
    return result ? 1 : 0;
#else
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd == -1) {
        return 0; // 失败
    }

    ssize_t bytes_read = read(fd, buffer, length);
    close(fd);
    return (bytes_read == (ssize_t)length) ? 1 : 0;
#endif
}

// 回退到更好的伪随机数生成（仅在安全随机数失败时使用）
static void fallback_random_init() {
    static int initialized = 0;
    if (!initialized) {
        // 使用多个熵源来改善随机性
        unsigned int seed = 0;

#ifdef _WIN32
        // Windows: 尝试使用性能计数器作为额外熵源
        LARGE_INTEGER counter;
        if (QueryPerformanceCounter(&counter)) {
            seed ^= (unsigned int)(counter.QuadPart & 0xFFFFFFFF);
            seed ^= (unsigned int)((counter.QuadPart >> 32) & 0xFFFFFFFF);
        }
#endif

        // 添加时间和进程ID作为熵源
        seed ^= (unsigned int)time(NULL);
        seed ^= (unsigned int)clock();

#ifdef _WIN32
        seed ^= (unsigned int)GetCurrentProcessId();
        seed ^= (unsigned int)GetTickCount();
#else
        seed ^= (unsigned int)getpid();
#endif

        srand(seed);
        initialized = 1;
    }
}

// 跨平台睡眠函数 - 秒
void sleepSeconds(int seconds)
{
    if (seconds <= 0) return;

#ifdef _WIN32
    Sleep(seconds * 1000);  // Windows: 毫秒
#else
    sleep(seconds);         // Unix/Linux: 秒
#endif
}

// 跨平台睡眠函数 - 毫秒
void sleepMilliseconds(int milliseconds)
{
    if (milliseconds <= 0) return;

#ifdef _WIN32
    Sleep(milliseconds);    // Windows: 毫秒
#else
    usleep(milliseconds * 1000);  // Unix/Linux: 微秒
#endif
}

// 跨平台睡眠函数 - 微秒
void sleepMicroseconds(int microseconds)
{
    if (microseconds <= 0) return;

#ifdef _WIN32
    // Windows没有直接的微秒睡眠，转换为毫秒（精度损失）
    int milliseconds = (microseconds + 999) / 1000;  // 向上取整
    if (milliseconds > 0) {
        Sleep(milliseconds);
    }
#else
    usleep(microseconds);   // Unix/Linux: 微秒
#endif
}

void setClientId(char** client_id)
{
    // 分配内存
    *client_id = malloc(37); // 36字符 + '\0'

    if (*client_id) {
        unsigned char random_bytes[16];

        // 尝试使用安全随机数生成器
        if (secure_random_bytes(random_bytes, 16)) {
            // 使用安全随机数生成UUID
            snprintf(*client_id, 37,
                "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                random_bytes[0], random_bytes[1], random_bytes[2], random_bytes[3],
                random_bytes[4], random_bytes[5],
                (random_bytes[6] & 0x0F) | 0x40,  // 版本4
                (random_bytes[7] & 0x3F) | 0x80,  // 变体位
                random_bytes[8], random_bytes[9],
                random_bytes[10], random_bytes[11], random_bytes[12],
                random_bytes[13], random_bytes[14], random_bytes[15]
            );
        } else {
            // 回退到伪随机数生成器（避免直接使用rand()）
            fallback_random_init();

            // 使用线性同余生成器替代rand()
            static unsigned long long uuid_state = 1;
            unsigned int random_values[4];

            for (int i = 0; i < 4; i++) {
                uuid_state = uuid_state * 1103515245ULL + 12345ULL;
                random_values[i] = (unsigned int)((uuid_state >> 16) & 0xFFFFFFFF);
            }

            // 生成伪UUID格式字符串
            snprintf(*client_id, 37,
                "%08x-%04x-%04x-%04x-%012llx",
                random_values[0],                                    // 8位十六进制
                random_values[1] & 0xFFFFU,                         // 4位十六进制
                (random_values[2] & 0x0FFFU) | 0x4000U,             // 4位十六进制，版本4
                (random_values[3] & 0x3FFFU) | 0x8000U,             // 4位十六进制，变体位
                ((unsigned long long)random_values[0] << 32) | random_values[1] // 12位十六进制
            );
        }

        // 转换为小写
        for (int i = 0; (*client_id)[i]; i++) {
            (*client_id)[i] = (char)tolower((unsigned char)(*client_id)[i]);
        }
    }
}

char* randomMacAddress()
{
    // 分配内存存储MAC地址字符串 (格式: xx:xx:xx:xx:xx:xx)
    char* mac_str = (char*)malloc(18 * sizeof(char));
    if (mac_str == NULL) {
        return NULL;
    }

    // 生成6个随机字节
    unsigned char mac_bytes[6];
    // 尝试使用安全随机数生成器
    if (secure_random_bytes(mac_bytes, 6)) {
        // 使用安全随机数
    } else {
        // 回退到伪随机数生成器（避免直接使用rand()）
        fallback_random_init();

        // 使用线性同余生成器的改进版本
        static unsigned long long state = 1;
        for (int i = 0; i < 6; i++) {
            state = state * 1103515245ULL + 12345ULL;
            mac_bytes[i] = (unsigned char)((state >> 16) & 0xFF);
        }
    }

    // 修正第一字节，确保最低位为0（单播地址）
    mac_bytes[0] = mac_bytes[0] & 0xFEU;  // 0xFE = 254 = 11111110

    // 格式化为字符串
    sprintf(mac_str, "%02x:%02x:%02x:%02x:%02x:%02x",
            mac_bytes[0], mac_bytes[1], mac_bytes[2],
            mac_bytes[3], mac_bytes[4], mac_bytes[5]);

    return mac_str;
}

// 获取当前时间字符串 (格式: yyyy-MM-dd HH:mm:ss)
char* getTime() {
    time_t rawtime;
    struct tm* timeinfo;
    char* time_str = (char*)malloc(20 * sizeof(char)); // "yyyy-MM-dd HH:mm:ss" + '\0'

    if (time_str == NULL) {
        return NULL;
    }

    // 获取当前时间
    time(&rawtime);
    timeinfo = localtime(&rawtime);

    // 格式化时间字符串
    strftime(time_str, 20, "%Y-%m-%d %H:%M:%S", timeinfo);

    return time_str;
}

// 格式化XML字符串的辅助函数
static int format_getTicket_xml(char* buffer, size_t size, const char* time_str) {
    return snprintf(buffer, size,
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
        time_str,
        HOST_NAME ? HOST_NAME : "",
        userIp ? userIp : "",
        macAddress ? macAddress : "",
        HOST_NAME ? HOST_NAME : "",
        acIp ? acIp : ""
    );
}

static int format_login_xml(char* buffer, size_t size, const char* time_str) {
    return snprintf(buffer, size,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<request>\n"
        "    <user-agent>%s</user-agent>\n"
        "    <client-id>%s</client-id>\n"
        "    <ticket>%s</ticket>\n"
        "    <local-time>%s</local-time>\n"
        "    <userid>%s</userid>"
        "    <passwd>%s</passwd>"
        "</request>",
        USER_AGENT ? USER_AGENT : "",
        clientId ? clientId : "",
        ticket ? ticket : "",
        time_str,
        usr,
        pwd
    );
}

// 创建XML payload字符串
char* createXMLPayload(const char* choose) {
    // 计算所需的缓冲区大小
    size_t buffer_size = 1024; // 初始大小，如果不够会动态扩展
    char* payload = (char*)malloc(buffer_size);
    int result;
    if (payload == NULL) {
        return NULL;
    }

    // 获取当前时间
    char* current_time = getTime();
    if (current_time == NULL) {
        free(payload);
        return NULL;
    }
    // 第一次尝试构建XML字符串
    if (!strcmp(choose, "getTicket"))
    {
        result = format_getTicket_xml(payload, buffer_size, current_time);
    }
    else
    {
        result = format_login_xml(payload, buffer_size, current_time);
    }

    // 检查是否需要更大的缓冲区
    if (result >= buffer_size) {
        buffer_size = result + 1;
        char* new_payload = (char*)realloc(payload, buffer_size);
        if (new_payload == NULL) {
            free(current_time);
            free(payload);
            return NULL;
        }
        payload = new_payload;

        // 重新格式化到更大的缓冲区
        if (!strcmp(choose, "getTicket"))
        {
            result = format_getTicket_xml(payload, buffer_size, current_time);
        }
        else
        {
            result = format_login_xml(payload, buffer_size, current_time);
        }
    }

    // 释放时间字符串内存
    free(current_time);

    return payload;
}

char* cleanCDATA(const char* text) {
    if (!text) return NULL;

    const char* cdata_start = "<![CDATA[";
    const char* cdata_end = "]]>";

    char* start = strstr(text, cdata_start);

    start += strlen(cdata_start);
    char* end = strstr(start, cdata_end);
    if (!end) {
        return strdup(text);
    }

    size_t len = end - start;
    char* result = malloc(len + 1);
    if (!result) return NULL;

    strncpy(result, start, len);
    result[len] = '\0';

    return result;
}