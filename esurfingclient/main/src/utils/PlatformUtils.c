#include "utils/PlatformUtils.h"
#include "utils/Watchdog.h"
#include "utils/Logger.h"

#include "cJSON/cJSON.h"

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

#else

#include <strings.h>

#ifdef __APPLE__
// get_exec_path() 用的 _NSGetExecutablePath() 在这个头里
#include <mach-o/dyld.h>
#endif

#endif

#ifdef __OPENWRT__
static const char config_file[] = "/etc/config/esurfingclient";
#else
#define DIALER_CONFIG_FILE "ESurfingClient.json"
static char config_file[PATH_MAX + 1 + sizeof(DIALER_CONFIG_FILE)];
#endif

#define WINDOWS_UA "CCTP/WinSVR5/1068"
#define LINUX_UA "CCTP/Linux64/1003"
#define OLD_ANDROID_UA "CCTP/android64_vpn/2093"
#define ANDROID_UA "CCTP/android11_64/2104"
#define IOS_UA "CCTP/iOSdy/4023"
#define MACOS_UA "CCTP/macdy/5019"

typedef struct
{
    char ip[IP_LEN];
    char name[NAME_LENGTH];
} adapter_t;

static const char s_default_cfg[] = "{\n"
                                    "   \"enabled\": false,\n"
                                    "   \"web_external_acc\": false,\n"
                                    "   \"log_lv\": 4,\n"
                                    "   \"log_dir\": \"./\",\n"
                                    "   \"conn_timeout\": 7,\n"
                                    "   \"op_timeout\": 10,\n"
                                    "   \"web_port\": 8888,\n"
                                    "   \"accounts\": [\n"
                                    "       {\n"
                                    "           \"username\": \"\",\n"
                                    "           \"password\": \"\",\n"
                                    "           \"channel\": 3,\n"
                                    "           \"mark\": \"\",\n"
                                    "           \"time_windows\": []\n"
                                    "       }\n"
                                    "   ]\n"
                                    "}\n";

static adapter_t* s_adaptor = NULL;
static uint8_t s_adaptor_count = 0;

static void get_adapters()
{
#ifdef _WIN32
    PIP_ADAPTER_INFO p_adapter_info = NULL;
    ULONG ul_out_buf_len = 0;
    if (GetAdaptersInfo(p_adapter_info, &ul_out_buf_len) == ERROR_BUFFER_OVERFLOW)
    {
        p_adapter_info = (PIP_ADAPTER_INFO)malloc(ul_out_buf_len);
        if (p_adapter_info && GetAdaptersInfo(p_adapter_info, &ul_out_buf_len) == NO_ERROR)
        {
            PIP_ADAPTER_INFO p_adapter = p_adapter_info;
            uint8_t cnt = 0;
            while (p_adapter)
            {
                adapter_t* new_adaptor = realloc(s_adaptor, sizeof(adapter_t) * (cnt + 1));
                if (!new_adaptor)
                {
                    LOG_ERROR("分配内存失败");
                    break;
                }
                s_adaptor = new_adaptor;
                snprintf(s_adaptor[cnt].name, NAME_LENGTH, "%s", p_adapter->Description);
                snprintf(s_adaptor[cnt].ip, IP_LEN, "%s", p_adapter->IpAddressList.IpAddress.String);
                LOG_VERBOSE("IP: %s", p_adapter->IpAddressList.IpAddress.String);
                p_adapter = p_adapter->Next;
                cnt++;
            }
            s_adaptor_count = cnt;
        }
    }
    if (p_adapter_info) free(p_adapter_info);
#else
    struct ifaddrs* ifaddrs_ptr, *ifa;
    if (getifaddrs(&ifaddrs_ptr) == 0)
    {
        uint8_t cnt = 0;
        for (ifa = ifaddrs_ptr; ifa; ifa = ifa->ifa_next)
        {
            if (!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET) continue;
            if (strcmp(ifa->ifa_name, "lo") == 0) continue;
            char ip[INET_ADDRSTRLEN];
            struct sockaddr_in *addr = (struct sockaddr_in*)ifa->ifa_addr;
            if (inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip)))
            {
                adapter_t* new_adaptor = realloc(s_adaptor, sizeof(adapter_t) * (cnt + 1));
                if (!new_adaptor)
                {
                    LOG_ERROR("分配内存失败");
                    break;
                }
                s_adaptor = new_adaptor;
                snprintf(s_adaptor[cnt].name, NAME_LENGTH, "%s", ifa->ifa_name);
                snprintf(s_adaptor[cnt].ip, IP_LEN, "%s", ip);
                cnt++;
            }
        }
        s_adaptor_count = cnt;
        freeifaddrs(ifaddrs_ptr);
    }
#endif
}

char* get_adapters_json()
{
    get_adapters();
    cJSON* root = cJSON_CreateObject();
    cJSON* adapters = cJSON_CreateArray();
    for (uint8_t i = 0; i < s_adaptor_count; i++)
    {
        if (strlen(s_adaptor[i].name) == 0) break;
        cJSON* adapter = cJSON_CreateObject();
        cJSON_AddStringToObject(adapter, "name", s_adaptor[i].name);
        cJSON_AddStringToObject(adapter, "ip", s_adaptor[i].ip);
        cJSON_AddItemToArray(adapters, adapter);
    }
    cJSON_AddItemToObject(root, "adapters", adapters);
    cJSON_AddStringToObject(root, "school_network_symbol", g_school_network_symbol);
    char* json = cJSON_Print(root);
    cJSON_Delete(root);
    return json;
}

static bool channel_str_eq(const char* a, const char* b)
{
#ifdef _WIN32
    return _stricmp(a, b) == 0;
#else
    return strcasecmp(a, b) == 0;
#endif
}

static uint8_t parse_channel_json(const cJSON* chn, const uint8_t cfg_no)
{
    if (chn == NULL)
    {
        LOG_WARN("配置 %" PRIu8 " channel 参数不存在, 使用默认通道 3 (Android)", cfg_no);
        return 3;
    }

    if (cJSON_IsNumber(chn))
    {
        const int value = chn->valueint;
        if (value >= 1 && value <= 5)
        {
            return (uint8_t)value;
        }
        LOG_WARN("配置 %" PRIu8 " channel 参数错误, 使用默认通道 3 (Android)", cfg_no);
        return 3;
    }

    if (cJSON_IsString(chn) && chn->valuestring != NULL)
    {
        const char* value = chn->valuestring;
        if (channel_str_eq(value, "windows") || channel_str_eq(value, "1"))
        {
            return 1;
        }
        if (channel_str_eq(value, "linux") || channel_str_eq(value, "2"))
        {
            return 2;
        }
        if (channel_str_eq(value, "android") || channel_str_eq(value, "3"))
        {
            return 3;
        }
        if (channel_str_eq(value, "ios") || channel_str_eq(value, "iphone") || channel_str_eq(value, "4"))
        {
            return 4;
        }
        if (channel_str_eq(value, "macos") || channel_str_eq(value, "mac") || channel_str_eq(value, "osx") || channel_str_eq(value, "5"))
        {
            return 5;
        }
    }

    LOG_WARN("配置 %" PRIu8 " channel 参数错误, 使用默认通道 3 (Android)", cfg_no);
    return 3;
}

static void apply_channel_ua(login_cfg_t* cfg, uint8_t cfg_no)
{
    switch (cfg->chn)
    {
    case 1:
        LOG_INFO("使用通道 1: Windows (暂未实现, 使用 Android 通道)");
        snprintf(cfg->user_agent, USER_AGENT_LEN, ANDROID_UA);
        break;
    case 2:
        LOG_INFO("使用通道 2: Linux");
        snprintf(cfg->user_agent, USER_AGENT_LEN, LINUX_UA);
        break;
    case 3:
        LOG_INFO("使用通道 3: Android");
        snprintf(cfg->user_agent, USER_AGENT_LEN, ANDROID_UA);
        break;
    case 4:
        LOG_INFO("使用通道 4: iOS");
        snprintf(cfg->user_agent, USER_AGENT_LEN, IOS_UA);
        break;
    case 5:
        LOG_INFO("使用通道 5: macOS");
        snprintf(cfg->user_agent, USER_AGENT_LEN, MACOS_UA);
        break;
    default:
        LOG_WARN("配置 %" PRIu8 " channel 参数错误, 使用默认通道 3 (Android)", cfg_no);
        cfg->chn = 3;
        snprintf(cfg->user_agent, USER_AGENT_LEN, ANDROID_UA);
        break;
    }
}

/**
 * @brief 将英文星期缩写转为周起始偏移 (0=周日 ... 6=周六)
 * @return 0-6, 失败返回 -1
 */
static int week_day_from_str(const char* str)
{
    if (!str) return -1;
    const char d0 = (char)tolower((unsigned char)str[0]);
    const char d1 = (char)tolower((unsigned char)str[1]);
    const char d2 = (char)tolower((unsigned char)str[2]);

    if (d0 == 'm' && d1 == 'o' && d2 == 'n') return 1;
    if (d0 == 't' && d1 == 'u' && d2 == 'e') return 2;
    if (d0 == 'w' && d1 == 'e' && d2 == 'd') return 3;
    if (d0 == 't' && d1 == 'h' && d2 == 'u') return 4;
    if (d0 == 'f' && d1 == 'r' && d2 == 'i') return 5;
    if (d0 == 's' && d1 == 'a' && d2 == 't') return 6;
    if (d0 == 's' && d1 == 'u' && d2 == 'n') return 0;
    return -1;
}

/**
 * @brief 解析 "mon 08:13" 格式
 * @param str 原始字符串
 * @param week_min 输出周分钟 (0-10079)
 * @return 是否合法
 */
static bool parse_week_time(const char* str, uint16_t* week_min)
{
    if (!str || strlen(str) != 9) return false;
    if (str[3] != ' ') return false;
    if (isdigit((unsigned char)str[4]) == 0 ||
        isdigit((unsigned char)str[5]) == 0 ||
        isdigit((unsigned char)str[7]) == 0 ||
        isdigit((unsigned char)str[8]) == 0 ||
        str[6] != ':')
    {
        return false;
    }

    const int day = week_day_from_str(str);
    if (day < 0) return false;

    const int hour = (str[4] - '0') * 10 + (str[5] - '0');
    const int minute = (str[7] - '0') * 10 + (str[8] - '0');
    if (hour > 23 || minute > 59) return false;

    if (week_min) *week_min = (uint16_t)(day * 1440 + hour * 60 + minute);
    return true;
}

/**
 * @brief 解析一个 time_windows 数组元素 { "start": "mon 08:13", "end": "sun 23:57" }
 * @param item cJSON 对象
 * @param win 输出窗口
 * @return 是否合法 (end <= start 时按跨周处理)
 */
static bool parse_time_window(const cJSON* item, time_window_t* win)
{
    if (!item || !cJSON_IsObject(item)) return false;

    const cJSON* start_item = cJSON_GetObjectItem(item, "start");
    const cJSON* end_item = cJSON_GetObjectItem(item, "end");
    if (!start_item || !cJSON_IsString(start_item) ||
        !end_item || !cJSON_IsString(end_item))
    {
        return false;
    }

    uint16_t start = 0;
    uint16_t end = 0;
    if (parse_week_time(start_item->valuestring, &start) == false ||
        parse_week_time(end_item->valuestring, &end) == false)
    {
        return false;
    }

    if (start == end) return false;

    if (end <= start)
    {
        end = (uint16_t)(end + WEEK_MINUTES);
    }

    if (win)
    {
        win->start_week_min = start;
        win->end_week_min = end;
    }
    return true;
}

/**
 * @brief 从 cJSON 数组解析 time_windows
 * @param arr cJSON 数组 (允许 NULL/空)
 * @param windows 输出窗口数组
 * @param count 输出窗口数量
 * @return 是否合法
 */
static bool parse_time_windows(const cJSON* arr, time_window_t* windows, uint8_t* count)
{
    if (count) *count = 0;

    if (arr == NULL)
    {
        return true;
    }
    if (cJSON_IsArray(arr) == false)
    {
        return false;
    }

    const int size = cJSON_GetArraySize(arr);
    if (size < 0 || size > MAX_TIME_WINDOWS) return false;

    for (int i = 0; i < size; i++)
    {
        time_window_t win;
        if (parse_time_window(cJSON_GetArrayItem(arr, i), &win) == false)
        {
            return false;
        }
        if (windows) windows[i] = win;
    }

    if (count) *count = (uint8_t)size;
    return true;
}

/**
 * @brief 解析 time_windows 并写入 login_cfg
 * @return 是否合法
 */
static bool apply_time_windows(const cJSON* item, login_cfg_t* cfg)
{
    if (parse_time_windows(item, cfg->time_windows, &cfg->time_window_count) == false)
    {
        return false;
    }
    cfg->has_time_control = cfg->time_window_count > 0;
    return true;
}

bool get_exec_path(char* path_array)
{
    if (path_array == NULL) return false;

#ifdef _WIN32
    char path[MAX_PATH];
    const DWORD len_d = GetModuleFileNameA(NULL, path, MAX_PATH);
    if (len_d == 0 || len_d >= MAX_PATH) return false;
    const uint16_t len = snprintf(path_array, PATH_MAX, "%s", safe_str(path));
    if ((size_t)len >= PATH_MAX) return false;
    return true;
#elif __linux__
    char path[PATH_MAX];
    const ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len <= 0 || len >= (ssize_t)sizeof(path)) return false;
    path[len] = '\0';
    const uint16_t n = snprintf(path_array, PATH_MAX, "%s", path);
    if ((size_t)n >= PATH_MAX) return false;
    return true;
#elif defined(__APPLE__)
    char path[PATH_MAX];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) != 0) return false;
    char* resolved = realpath(path, NULL);
    if (!resolved) return false;
    const uint16_t n = snprintf(path_array, PATH_MAX, "%s", resolved);
    free(resolved);
    if ((size_t)n >= PATH_MAX) return false;
    return true;
#else
    (void)path_array;
    return false;
#endif
}

bool get_exec_dir(char* dir_array)
{
    char path[PATH_MAX];
    if (get_exec_path(path) == false) return false;

    char* last = strrchr(path, '/');
#ifdef _WIN32
    char* last_win = strrchr(path, '\\');
    if (last_win != NULL && (last == NULL || last_win > last)) last = last_win;
#endif
    if (last == NULL) return false;
    *last = '\0';

    const uint16_t len = snprintf(dir_array, PATH_MAX, "%s", path);
    if ((size_t)len >= PATH_MAX) return false;
    return true;
}

#ifndef _WIN32
/** @brief 启动时的父进程号 (用来判断父进程是否已经没了) */
static pid_t s_parent_pid = 0;
#endif

void record_parent_pid(void)
{
#ifndef _WIN32
    s_parent_pid = getppid();
#endif
}

bool parent_process_alive(void)
{
#ifdef _WIN32
    /* Windows 没有等价机制 (要彻底解决得用 Job Object) */
    return true;
#else
    /* 没登记过就不做判断, 免得误杀 */
    if (s_parent_pid == 0) return true;

    /*
     * 关键: 是"和启动时那个父进程号比", 而不是"看是不是 1"。
     * 用 setsid 之类方式主动脱离终端的进程, 父进程本来就可能是 init,
     * 按"是不是 1"判断会把它误当成孤儿
     */
    return getppid() == s_parent_pid;
#endif
}

char* xml_parser(const char* xml_data, const char* tag)
{
    if (xml_data == NULL || tag == NULL) return NULL;

    char start_tag[256];
    snprintf(start_tag, sizeof(start_tag), "<%s>", tag);

    char end_tag[256];
    snprintf(end_tag, sizeof(end_tag), "</%s>", tag);

    const char* start_pos = strstr(xml_data, start_tag);
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

bytes_t str2bytes(const char* str)
{
    bytes_t ba = {0};
    if (!str) return ba;
    ba.length = strlen(str);
    ba.data = (uint8_t*)malloc(ba.length);
    if (ba.data) memcpy(ba.data, str, ba.length);
    return ba;
}

uint64_t str2uint64(const char* str)
{
    if (!str) return 0;
    while (isspace(*str)) str++;
    if (*str == '\0') return 0;
    char* end_ptr;
    errno = 0;
    const uint64_t value = strtoll(str, &end_ptr, 10);
    if (errno == ERANGE) return 0;
    if (end_ptr == str) return 0;
    while (isspace(*end_ptr)) end_ptr++;
    if (*end_ptr != '\0') return 0;
    return value;
}

char* uint642str(const uint64_t num)
{
    char* result = malloc(22);
    if (!result) return NULL;
    snprintf(result, 22, "%" PRIu64, num);
    return result;
}

uint64_t get_cur_tm_ms()
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
    if (gettimeofday(&tv, NULL) != 0) return 0;
    return tv.tv_sec * 1000LL + tv.tv_usec / 1000LL;
#endif
}

void get_rand_bytes(uint8_t* buf, const size_t len)
{
#ifdef _WIN32
    HCRYPTPROV h_crypt_prov;
    if (!CryptAcquireContext(&h_crypt_prov, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) return;
    CryptGenRandom(h_crypt_prov, len, buf);
    CryptReleaseContext(h_crypt_prov, 0);
#else
    const int fd = open("/dev/urandom", O_RDONLY);
    if (fd == -1) return;
    read(fd, buf, len);
    close(fd);
#endif
}

void sleep_ms(const uint64_t ms, const bool can_stop)
{
    if (ms == 0) return;

    /**
     * 睡眠是最常见的"长时间没动静", 但它【不是卡死】—— 它本身就是在声明
     * "我会安静这么久"。所以在这里打卡, 预算就是这次睡眠的时长。
     *
     * 这一处不能省: 认证失败后的退避会睡 60 秒到 30 分钟 (见 DialerClient.c 的
     * table[]), 不打卡的话看门狗会把它当成卡死, 于是"认证失败 -> 退避 ->
     * 被杀 -> 重启 -> 再失败"变成杀循环。
     *
     * 注意它与网络请求处的打卡是配套的: 这里可能把预算改小 (睡 1 秒),
     * 而紧接着的网络请求会自己再打一次卡把预算调回来, 所以不会误杀。
     */
    watchdog_pet(ms > UINT32_MAX ? UINT32_MAX : (uint32_t)ms);

    if (can_stop)
    {
        uint64_t elapsed = 0;

        while (elapsed < ms && g_thread_keep_alive && g_stop_requested == 0)
        {
            if (tl_thread_idx > -1)
            {
                if (g_prog_status[tl_thread_idx].runtime_status.is_running == false || g_prog_status[tl_thread_idx].runtime_status.is_need_reauth)
                {
                    return;
                }
            }
            else
            {
                if (g_need_exit)
                {
                    return;
                }
            }
            const uint64_t SEGMENT_MS = 100;
            const uint64_t sleep_time = ms - elapsed < SEGMENT_MS ? ms - elapsed : SEGMENT_MS;

#ifdef _WIN32
            Sleep(sleep_time);
#else
            usleep(sleep_time * 1000);
#endif
            elapsed += sleep_time;
        }
    }
    else
    {
#ifdef _WIN32
        Sleep(ms);
#else
        usleep(ms * 1000);
#endif
    }
}

void get_fmt_time(char* buf, const TimeFormat fmt)
{
    time_t raw_tm;
    if (time(&raw_tm) == (time_t) - 1)
    {
        fprintf(stderr, "ERROR: 获取系统时间失败\n");
        return;
    }
    struct tm local_tm;
#ifdef _WIN32
    if (localtime_s(&local_tm, &raw_tm) != 0)
    {
        fprintf(stderr, "ERROR: 时间转换失败\n");
        return;
    }
#else
    if (localtime_r(&raw_tm, &local_tm) == NULL)
    {
        fprintf(stderr, "ERROR: 时间转换失败\n");
        return;
    }
#endif
    switch (fmt)
    {
    case CONSOLE_FORMAT:
        if (strftime(buf, 32, "%Y-%m-%d %H:%M:%S", &local_tm) == 0)
        {
            fprintf(stderr, "ERROR: 格式化时间失败\n");
            return;
        }
        return;
    case FILE_FORMAT:
        if (strftime(buf, 32, "%Y%m%d-%H%M%S", &local_tm) == 0)
        {
            fprintf(stderr, "ERROR: 格式化时间失败\n");
        }
    }
}

const char* safe_str(const char* str)
{
    return str ? str : "";
}

char* create_xml_payload(const XmlChoose choose)
{
    char cur_tm[32];
    get_fmt_time(cur_tm, CONSOLE_FORMAT);
    static char xml[XML_BUFFER_SIZE] = "";
    LOG_DEBUG("XML 选择代码: %d", choose);
    uint16_t xml_len = 0;
    switch (choose)
    {
    case GET_TICKET:
        xml_len = snprintf(xml, XML_BUFFER_SIZE,
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
            "</request>\n",
            safe_str(g_prog_status[tl_thread_idx].login_cfg.user_agent),
            safe_str(g_prog_status[tl_thread_idx].auth_cfg.client_id),
            safe_str(cur_tm),
            safe_str(g_prog_status[tl_thread_idx].auth_cfg.host_name),
            safe_str(g_prog_status[tl_thread_idx].auth_cfg.client_ip),
            safe_str(g_prog_status[tl_thread_idx].auth_cfg.mac_addr),
            safe_str(g_prog_status[tl_thread_idx].auth_cfg.ostag),
            safe_str(g_prog_status[tl_thread_idx].auth_cfg.ac_ip)
        );
        break;
    case LOGIN:
        xml_len = snprintf(xml, XML_BUFFER_SIZE,
            "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
            "<request>\n"
            "    <user-agent>%s</user-agent>\n"
            "    <client-id>%s</client-id>\n"
            "    <ticket>%s</ticket>\n"
            "    <local-time>%s</local-time>\n"
            "    <userid>%s</userid>\n"
            "    <passwd>%s</passwd>\n"
            "</request>\n",
            safe_str(g_prog_status[tl_thread_idx].login_cfg.user_agent),
            safe_str(g_prog_status[tl_thread_idx].auth_cfg.client_id),
            safe_str(g_prog_status[tl_thread_idx].auth_cfg.ticket),
            safe_str(cur_tm),
            safe_str(g_prog_status[tl_thread_idx].login_cfg.usr),
            safe_str(g_prog_status[tl_thread_idx].login_cfg.pwd)
        );
        break;
    case HEART_BEAT:
    case TERM:
        xml_len = snprintf(xml, XML_BUFFER_SIZE,
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
            "</request>\n",
            safe_str(g_prog_status[tl_thread_idx].login_cfg.user_agent),
            safe_str(g_prog_status[tl_thread_idx].auth_cfg.client_id),
            safe_str(cur_tm),
            safe_str(g_prog_status[tl_thread_idx].auth_cfg.host_name),
            safe_str(g_prog_status[tl_thread_idx].auth_cfg.client_ip),
            safe_str(g_prog_status[tl_thread_idx].auth_cfg.ticket),
            safe_str(g_prog_status[tl_thread_idx].auth_cfg.mac_addr),
            safe_str(g_prog_status[tl_thread_idx].auth_cfg.ostag)
        );
        break;
    default:
        LOG_ERROR("XML 选择代码错误");
        return NULL;
    }
    if (xml_len <= 0)
    {
        LOG_ERROR("XML 创建失败");
        return NULL;
    }
    if (xml_len >= XML_BUFFER_SIZE)
    {
        LOG_ERROR("XML 内容过长 (需要 %d 字节，但缓冲区只有 %d 字节)", xml_len + 1, XML_BUFFER_SIZE);
        return NULL;
    }
    LOG_DEBUG("创建 XML 完成");
    if (choose != LOGIN)
    {
        LOG_DEBUG("XML 内容为:\n%s", xml);
    }
    return xml;
}

char* extract_between_tags(const char* text, const char* start_tag, const char* end_tag)
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

char* clean_CDATA(const char* text)
{
    return extract_between_tags(text, "<![CDATA[", "]]>");
}

/* ------------------------------------------------------------------
 * 配置文件的读写与补全
 * ------------------------------------------------------------------ */

/**
 * @brief 把配置对象写回配置文件
 *
 * 网页界面保存与"启动时补全配置"都走这里: 输出的是 cJSON 的格式 (Tab 缩进),
 * 与网页界面保存出来的完全一致
 * @param cfg_json 配置对象
 * @return 是否写成功
 */
static bool write_cfg_json(const cJSON* cfg_json)
{
    char* cfg_text = cJSON_Print(cfg_json);
    if (cfg_text == NULL)
    {
        LOG_ERROR("配置内容序列化失败");
        return false;
    }

    FILE* cfg_file = fopen(config_file, "w");
    if (!cfg_file)
    {
        LOG_ERROR("无法生成文件: %s", config_file);
        free(cfg_text);
        return false;
    }
    fprintf(cfg_file, "%s", cfg_text);
    fclose(cfg_file);

    free(cfg_text);
    return true;
}

/** @brief 补全时用得上的默认值类型 */
typedef enum
{
    CFG_DEF_BOOL = 0,
    CFG_DEF_NUM,
    CFG_DEF_STR,
    CFG_DEF_ARR
} cfg_def_type_t;

/** @brief 一个可以补的参数 */
typedef struct
{
    /** @brief 参数名 */
    const char* name;
    /** @brief 默认值的类型 */
    cfg_def_type_t type;
    /** @brief 默认值 (布尔) */
    bool boolean;
    /** @brief 默认值 (数字) */
    int number;
    /** @brief 默认值 (字符串) */
    const char* str;
    /** @brief 默认值的单位 (只用于日志, 可为空) */
    const char* unit;
} cfg_def_t;

/**
 * @brief 配置文件顶层的默认参数
 *
 * ⚠️ 必须与 config/ESurfingClient.json 以及 s_default_cfg 保持一致:
 *    缺哪个参数, 用户下次打开配置文件就会看到它被补上, 补上的值就是这里的默认值
 */
static const cfg_def_t s_cfg_defaults[] = {
    {"enabled",          CFG_DEF_BOOL, false, 0, NULL, NULL},
    {"web_external_acc", CFG_DEF_BOOL, DEFAULT_WEB_EXTERNAL_ACC, 0, NULL, NULL},
    {"log_lv",           CFG_DEF_NUM, false, LOG_LEVEL_INFO, NULL, NULL},
    {"log_dir",          CFG_DEF_STR, false, 0, DEFAULT_LOG_DIR, NULL},
    {"conn_timeout",     CFG_DEF_NUM, false, DEFAULT_CONN_TIMEOUT, NULL, "秒"},
    {"op_timeout",       CFG_DEF_NUM, false, DEFAULT_OP_TIMEOUT, NULL, "秒"},
    {"web_port",         CFG_DEF_NUM, false, DEFAULT_WEB_PORT, NULL, NULL},
};

/** @brief 账号里的默认参数 */
static const cfg_def_t s_account_defaults[] = {
    {"username",     CFG_DEF_STR, false, 0, "", NULL},
    {"password",     CFG_DEF_STR, false, 0, "", NULL},
    {"channel",      CFG_DEF_NUM, false, DEFAULT_CHANNEL, NULL, NULL},
    {"mark",         CFG_DEF_STR, false, 0, "", NULL},
    {"time_windows", CFG_DEF_ARR, false, 0, NULL, NULL},
};

/**
 * @brief 把默认值渲染成日志里显示的文本
 * @param def 默认参数
 * @param out 输出缓冲
 * @param len 缓冲长度
 */
static void cfg_def_value_str(const cfg_def_t* def, char* out, const size_t len)
{
    switch (def->type)
    {
    case CFG_DEF_BOOL:
        snprintf(out, len, "%s", def->boolean ? "true" : "false");
        break;
    case CFG_DEF_NUM:
        if (def->unit != NULL) snprintf(out, len, "%d %s", def->number, def->unit);
        else snprintf(out, len, "%d", def->number);
        break;
    case CFG_DEF_STR:
        snprintf(out, len, "\"%s\"", safe_str(def->str));
        break;
    default:
        snprintf(out, len, "[]");
        break;
    }
}

/**
 * @brief 把缺的参数补进一个配置对象里
 *
 * 只补【缺的键】: 已经写过的 (哪怕写的是空值或者不合法的值) 一律不动 ——
 * 那是用户自己的选择, 该报错就报错, 不该被程序悄悄改成"看起来正常"的样子
 * @param obj 配置对象 (原地补全)
 * @param defs 默认值表
 * @param def_cnt 默认值数量
 * @param who 日志前缀 (如 "配置里" / "配置 2 ")
 * @return 补上的参数个数
 */
static uint8_t complete_cfg_obj(cJSON* obj, const cfg_def_t* defs, const size_t def_cnt, const char* who)
{
    uint8_t added = 0;

    for (size_t i = 0; i < def_cnt; i++)
    {
        const cfg_def_t* def = &defs[i];

        if (cJSON_GetObjectItem(obj, def->name) != NULL) continue;

        bool ok = false;
        switch (def->type)
        {
        case CFG_DEF_BOOL: ok = cJSON_AddBoolToObject(obj, def->name, def->boolean) != NULL; break;
        case CFG_DEF_NUM:  ok = cJSON_AddNumberToObject(obj, def->name, def->number) != NULL; break;
        case CFG_DEF_STR:  ok = cJSON_AddStringToObject(obj, def->name, safe_str(def->str)) != NULL; break;
        default:           ok = cJSON_AddArrayToObject(obj, def->name) != NULL; break;
        }

        // 补不上 (内存不够之类) 就按老路子走, 后面会打"参数不存在"的日志
        if (ok == false) continue;

        char value[TIME_WINDOW_STR_LEN];
        cfg_def_value_str(def, value, sizeof(value));
        LOG_INFO("%s缺少 %s 参数, 已按默认值 %s 补上", who, def->name, value);
        added++;
    }

    return added;
}

/**
 * @brief 补全配置文件里缺失的参数
 *
 * 手写的配置常常漏参数, 尤其是新版本加上去的 —— 不翻更新日志根本不知道有这回事。
 * 这里在读取配置的时候把缺的按默认值补上, 用户下次打开配置文件就能看见自己漏了什么。
 *
 * 补完由调用方写回文件; 写不回去也不影响本次运行 (值已经在内存里了)
 * @param cfg_json 配置对象 (原地补全)
 * @return 补上的参数个数
 */
static uint8_t complete_cfg(cJSON* cfg_json)
{
    uint8_t added = complete_cfg_obj(cfg_json, s_cfg_defaults,
        sizeof(s_cfg_defaults) / sizeof(s_cfg_defaults[0]), "配置里");

    const cJSON* accounts = cJSON_GetObjectItem(cfg_json, "accounts");
    if (accounts != NULL && cJSON_IsArray(accounts))
    {
        const int cnt = cJSON_GetArraySize(accounts);
        for (int i = 0; i < cnt; i++)
        {
            cJSON* account = cJSON_GetArrayItem(accounts, i);
            if (account == NULL || cJSON_IsObject(account) == false) continue;

            char who[32];
            snprintf(who, sizeof(who), "配置 %d ", i + 1);
            added += complete_cfg_obj(account, s_account_defaults,
                sizeof(s_account_defaults) / sizeof(s_account_defaults[0]), who);
        }
    }
    else if (accounts == NULL)
    {
        /**
         * accounts 整个缺了: 补一份账号模板, 用户把账号密码填上就能跑
         *
         * 模板里的字段不再逐个打"补上了", 一次刷五行日志没人爱看
         */
        cJSON* new_accounts = cJSON_AddArrayToObject(cfg_json, "accounts");
        cJSON* account = cJSON_CreateObject();
        if (new_accounts != NULL && account != NULL)
        {
            cJSON_AddStringToObject(account, "username", "");
            cJSON_AddStringToObject(account, "password", "");
            cJSON_AddNumberToObject(account, "channel", DEFAULT_CHANNEL);
            cJSON_AddStringToObject(account, "mark", "");
            cJSON_AddArrayToObject(account, "time_windows");
            cJSON_AddItemToArray(new_accounts, account);

            LOG_INFO("配置里缺少 accounts 参数, 已补上一份账号模板 (把 username 与 password 填上)");
            added++;
        }
        else if (account != NULL)
        {
            cJSON_Delete(account);
        }
    }

    return added;
}

#ifndef __OPENWRT__

bool save_cfg(const char* configs_str)
{
    LOG_INFO("保存配置中");
    LOG_INFO("仅会保存第一个可用配置");

    cJSON* configs = cJSON_Parse(configs_str);
    if (!configs)
    {
        LOG_ERROR("配置 JSON 解析失败");
        return false;
    }

    const bool saved = write_cfg_json(configs);
    cJSON_Delete(configs);

    return saved;
}

const char* get_config_file_path(void)
{
    return config_file;
}

#endif

/** @brief 是否处于"只列举账号"模式 */
static bool s_list_only = false;

/**
 * @brief 处理无法继续的配置问题
 *
 * 单进程模式下挂起等待人工处理: 配置没填好时反复重启只会刷屏,
 * 等用户改完配置手动重启即可。
 *
 * 挂起期间必须仍然能被 Ctrl+C / 关窗口 / 服务停止打断 —— 有专门的回归用例
 * 盯着这一点 (test-windows-runtime.sh 第 8 节), 因为漏掉时会表现成
 * "程序假死 + 日志不落盘", 只能去任务管理器杀进程。
 *
 * 以下情况直接返回失败, 由调用方退出:
 * - 列举账号: init 脚本调用, 挂起会卡住开机
 * - 认证 / Web / 监管进程: 它们都在外部监管者 (procd / systemd / SCM) 之下,
 *   退出后按 respawn 或重启策略处理。挂住的进程监管者是不会重启的,
 *   那样只会看起来"服务在跑"却什么都不干
 */
static void cfg_halt()
{
    if (s_list_only ||
        g_prog_role == ROLE_AUTH ||
        g_prog_role == ROLE_WEB ||
        g_prog_role == ROLE_SUPERVISOR)
    {
        return;
    }

    while (true)
    {
        if (g_need_exit) return;

        /**
         * 信号处理函数现在只置 g_stop_requested (不再直接调 shut() —— 那里面的
         * join / 打日志 / rename / exit 都不是 async-signal-safe 的),
         * 而 g_need_exit 只有 shut() 会置。单进程模式下 shut() 要等 load_cfg()
         * 返回之后才调用, 所以这里只等 g_need_exit 的话会永远等不到:
         * Ctrl+C 毫无反应, 关窗口被系统强杀, 日志也就不会被改名。
         * 返回后由调用方 (work() 里的 shut(1)) 走正常关闭流程。
         */
        if (g_stop_requested) return;

        sleep_ms(10000, true);
    }
}

const char* get_config_path(void)
{
    return config_file;
}

bool load_cfg()
{
    g_cfg_loaded = false;
    /**
     * 桌面分支直接写 g_prog_status[0], 这里保证至少有一格可用
     * (OpenWrt 分支后面会按配置数重新分配)
     */
    if (g_prog_status == NULL)
    {
        g_prog_status = calloc(1, sizeof(prog_status_t));
        if (g_prog_status == NULL)
        {
            LOG_FATAL("分配内存失败");
            cfg_halt();
            return false;
        }
    }
#ifndef __OPENWRT__

    char dir[PATH_MAX];
    if (get_exec_dir(dir) == false)
    {
        LOG_ERROR("获取可执行文件路径失败, 请检查权限后重启");
        cfg_halt();
        return false;
    }
    snprintf(config_file, PATH_MAX + 1 + sizeof(DIALER_CONFIG_FILE), "%s%c%s", safe_str(dir), SEP, DIALER_CONFIG_FILE);

#endif

    FILE* cfg_file = fopen(config_file, "r");
    if (!cfg_file || fgetc(cfg_file) == EOF)
    {
        LOG_ERROR("无法打开配置文件或配置文件为空: %s", config_file);
        LOG_INFO("创建新的默认配置文件");
        FILE* new_cfg = fopen(config_file, "w");
        if (!new_cfg)
        {
            LOG_FATAL("无法生成文件: %s, 请检查权限后重启", config_file);
            cfg_halt();
            return false;
        }
        fprintf(new_cfg, "%s", s_default_cfg);
        fclose(new_cfg);
        LOG_INFO("创建完成, 请在 %s 填写账号数据, 然后重启", config_file);
        cfg_halt();
        return false;
    }

    fseek(cfg_file, 0, SEEK_END);
    const long len = ftell(cfg_file);
    fseek(cfg_file, 0, SEEK_SET);

    char* cfg_data = malloc(len + 1);
    fread(cfg_data, 1, len, cfg_file);
    cfg_data[len] = '\0';
    fclose(cfg_file);

    cJSON* cfg_json = cJSON_Parse(cfg_data);
    free(cfg_data);
    if (!cfg_json)
    {
        LOG_FATAL("JSON 解析失败, 请检查后重启");
        cfg_halt();
        return false;
    }

    /**
     * 先把配置补全, 再往下解析
     *
     * 放在这里是为了后面每一处解析都能拿到完整的对象; 补全后的内容要写回文件,
     * 用户下次打开配置就能看见自己漏了什么 (写不回去也不影响本次运行, 值已经在内存里)
     */
    const uint8_t completed = complete_cfg(cfg_json);
    if (completed > 0)
    {
        LOG_INFO("配置文件缺少参数, 已按默认值补全 %" PRIu8 " 个", completed);
        if (write_cfg_json(cfg_json))
        {
            LOG_INFO("补全后的配置已写回 %s", config_file);
        }
        else
        {
            LOG_WARN("补全后的配置写回失败 (配置文件可能是只读的?), 本次仍按补全后的值运行");
        }
    }

    /**
     * 下面这些"参数不存在"的分支现在基本走不到了 (上面已经补全),
     * 留着是给补全时分配内存失败之类的极端情况兜底
     */
    const cJSON* enabled = cJSON_GetObjectItem(cfg_json, "enabled");
    if (enabled == NULL)
    {
        LOG_WARN("enabled 参数不存在, 请填写后重启程序");
        g_prog_enabled = false;
        cfg_halt();
        return false;
    }
    if (cJSON_IsFalse(enabled))
    {
        LOG_WARN("配置文件中禁用了程序启动, 请开启后重启程序");
        g_prog_enabled = false;
        cfg_halt();
        return false;
    }
    g_prog_enabled = true;

    const cJSON* log_lv = cJSON_GetObjectItem(cfg_json, "log_lv");
    if (log_lv)
    {
        if (cJSON_IsNumber(log_lv))
        {
            set_logger_level(log_lv->valueint);
        }
        else
        {
            LOG_WARN("log_lv 参数不正确, 使用默认参数 (INFO)");
        }
    }
    else
    {
        LOG_WARN("log_lv 参数不存在, 使用默认参数 (INFO)");
    }

    /**
     * 日志目录
     *
     * 配置里的 log_dir 是【基目录】, 日志放在它下面的 logs 里 (默认就是程序所在目录下的
     * logs, OpenWrt 上则固定为 /var/log/esurfing/logs)。
     * 必须在这里才能定下来: 日志系统是先起来再读配置的 (配置读错了更要有日志),
     * set_logger_dir 会把已经写下的那几行一起搬到新目录去。
     * OpenWrt 上基目录是写死的, 那边只有写了别的目录才会提一句。
     */
    const cJSON* log_dir = cJSON_GetObjectItem(cfg_json, "log_dir");
    if (log_dir)
    {
        if (cJSON_IsString(log_dir) && log_dir->valuestring != NULL)
        {
            set_logger_dir(log_dir->valuestring);
        }
        else
        {
            LOG_WARN("log_dir 参数不正确, 使用默认值 (%s)", get_logger_dir_cfg());
        }
    }
    else
    {
        LOG_DEBUG("log_dir 参数不存在, 使用默认值 (%s)", get_logger_dir_cfg());
    }

    const cJSON* conn_timeout = cJSON_GetObjectItem(cfg_json, "conn_timeout");
    if (conn_timeout)
    {
        if (cJSON_IsNumber(conn_timeout))
        {
            g_conn_timeout = conn_timeout->valueint;
        }
        else
        {
            LOG_WARN("conn_timeout 参数不正确, 使用默认参数 (%d 秒)", DEFAULT_CONN_TIMEOUT);
        }
    }
    else
    {
        LOG_WARN("conn_timeout 参数不存在, 使用默认参数 (%d 秒)", DEFAULT_CONN_TIMEOUT);
    }


    const cJSON* op_timeout = cJSON_GetObjectItem(cfg_json, "op_timeout");
    if (op_timeout)
    {
        if (cJSON_IsNumber(op_timeout))
        {
            g_op_timeout = op_timeout->valueint;
        }
        else
        {
            LOG_WARN("op_timeout 参数不正确, 使用默认参数 (%d 秒)", DEFAULT_OP_TIMEOUT);
        }
    }
    else
    {
        LOG_WARN("op_timeout 参数不存在, 使用默认参数 (%d 秒)", DEFAULT_OP_TIMEOUT);
    }

    /**
     * Web 服务端口与是否允许外部访问
     *
     * 只有桌面端有 Web 服务 (OpenWrt 上不编译 WebServer.c), 那边这两个参数
     * 解析出来也没人用, 但配置文件的格式是同一套, 这里就一起读掉
     */
    const cJSON* web_port = cJSON_GetObjectItem(cfg_json, "web_port");
    if (web_port)
    {
        if (cJSON_IsNumber(web_port) && web_port->valueint >= 1 && web_port->valueint <= 65535)
        {
            g_web_port = (uint16_t)web_port->valueint;
        }
        else
        {
            LOG_WARN("web_port 参数不正确 (应为 1 - 65535), 使用默认参数 (%d)", DEFAULT_WEB_PORT);
        }
    }
    else
    {
        LOG_DEBUG("web_port 参数不存在, 使用默认参数 (%d)", DEFAULT_WEB_PORT);
    }

    const cJSON* web_external_acc = cJSON_GetObjectItem(cfg_json, "web_external_acc");
    if (web_external_acc)
    {
        if (cJSON_IsBool(web_external_acc))
        {
            g_web_external_acc = cJSON_IsTrue(web_external_acc);
        }
        else
        {
            LOG_WARN("web_external_acc 参数不正确 (应为 true / false), 使用默认参数 (关闭)");
        }
    }
    else
    {
        LOG_DEBUG("web_external_acc 参数不存在, 使用默认参数 (关闭)");
    }

    const cJSON* accounts = cJSON_GetObjectItem(cfg_json, "accounts");
    if (accounts == NULL || cJSON_IsArray(accounts) == false || cJSON_GetArraySize(accounts) == 0)
    {
        LOG_FATAL("没有找到账号数据, 请添加后重启程序");
        cJSON_Delete(cfg_json);
        cfg_halt();
        return false;
    }

    const uint8_t cnt = cJSON_GetArraySize(accounts);

    int8_t valid_cnt = 0;

#ifdef __OPENWRT__
    if (g_prog_account != 0)
    {
        LOG_INFO("OpenWRT 环境, 本次仅使用配置 %" PRIu8, g_prog_account);
    }
    else
    {
        LOG_INFO("OpenWRT 环境, 会尝试加载所有有效配置");
    }

    prog_status_t* new_prog_status = realloc(g_prog_status, sizeof(prog_status_t) * cnt);
    if (new_prog_status)
    {
        g_prog_status = new_prog_status;
        memset(g_prog_status, 0, sizeof(prog_status_t) * cnt);
    }
    else
    {
        LOG_FATAL("重分配内存失败");
        return false;
    }

    bool use_cus_mark = false;

    for (uint8_t i = 0, valid_i = 0; i < cnt; i++)
    {
        const cJSON* account = cJSON_GetArrayItem(accounts, i);

        const cJSON* usr = cJSON_GetObjectItem(account, "username");
        const cJSON* pwd = cJSON_GetObjectItem(account, "password");
        const cJSON* chn = cJSON_GetObjectItem(account, "channel");
        const cJSON* mark = cJSON_GetObjectItem(account, "mark");
        const cJSON* time_windows_item = cJSON_GetObjectItem(account, "time_windows");

        // 检查账号
        if (usr == NULL)
        {
            LOG_WARN("配置 %" PRIu8 " username 参数不存在, 跳过当前配置", i + 1);
            continue;
        }
        if (usr->valuestring[0] == '\0')
        {
            LOG_WARN("配置 %" PRIu8 " username 参数为空, 跳过当前配置", i + 1);
            continue;
        }

        // 检查密码
        if (pwd == NULL)
        {
            LOG_WARN("配置 %" PRIu8 " password 参数不存在, 跳过当前配置", i + 1);
            continue;
        }
        if (pwd->valuestring[0] == '\0')
        {
            LOG_WARN("配置 %" PRIu8 " password 参数为空, 跳过当前配置", i + 1);
            continue;
        }

        // 检查时间控制字段
        if (apply_time_windows(time_windows_item, &g_prog_status[valid_i].login_cfg) == false)
        {
            LOG_FATAL("配置 %" PRIu8 " time_windows 非法, 应为 [{ \"start\": \"mon 08:13\", \"end\": \"mon 23:57\" }, ...]", i + 1);
            cJSON_Delete(cfg_json);
            return false;
        }

        snprintf(g_prog_status[valid_i].login_cfg.usr, USR_LEN, "%s", safe_str(usr->valuestring));
        snprintf(g_prog_status[valid_i].login_cfg.pwd, PWD_LEN, "%s", safe_str(pwd->valuestring));

        g_prog_status[valid_i].login_cfg.chn = parse_channel_json(chn, i + 1);
        apply_channel_ua(&g_prog_status[valid_i].login_cfg, i + 1);

        LOG_DEBUG("使用 UA: %s", g_prog_status[valid_i].login_cfg.user_agent);
        LOG_DEBUG("当前使用下标: %" PRIu8, valid_i);

        // 检查标记值
        if (mark == NULL)
        {
            if (use_cus_mark)
            {
                LOG_WARN("其它配置使用了自定义标记值, 但配置 %" PRIu8 " 未填写, 将跳过该配置", i + 1);
                continue;
            }
            g_prog_status[valid_i].login_cfg.mark = 0x100 + valid_i * 0x100;
            LOG_DEBUG("使用自动标记值: %" PRIu32 " (0x%x)", g_prog_status[valid_i].login_cfg.mark, g_prog_status[valid_i].login_cfg.mark);
            LOG_DEBUG("当前使用下标: %" PRIu8, valid_i);
        }
        else
        {
            if (use_cus_mark && mark->valuestring[0] == '\0')
            {
                LOG_WARN("其它配置使用了自定义标记值, 但配置 %" PRIu8 " 未填写, 将跳过该配置", i + 1);
                continue;
            }
            if (mark->valuestring[0] != '\0')
            {
                g_prog_status[valid_i].login_cfg.mark = strtoul(mark->valuestring, NULL, 16);
                g_prog_status[valid_i].login_cfg.use_cus_mark = true;
                use_cus_mark = true;
                LOG_DEBUG("使用自定义标记值: %" PRIu32 " (0x%x)", g_prog_status[valid_i].login_cfg.mark, g_prog_status[valid_i].login_cfg.mark);
                LOG_DEBUG("当前使用下标: %" PRIu8, valid_i);
            }
            else
            {
                g_prog_status[valid_i].login_cfg.mark = 0x100 + valid_i * 0x100;
                LOG_DEBUG("使用自动标记值: %" PRIu32 " (0x%x)", g_prog_status[valid_i].login_cfg.mark, g_prog_status[valid_i].login_cfg.mark);
                LOG_DEBUG("当前使用下标: %" PRIu8, valid_i);
            }
        }

        // g_prog_status[valid_i].login_cfg.auto_start = auto_start->valueint;

        g_prog_status[valid_i].login_cfg.idx = i + 1;
        LOG_INFO("配置 %" PRIu8 " 可用, 将会尝试使用", i + 1);
        valid_cnt++;
        valid_i++;
    }

#else

    if (g_prog_account != 0)
    {
        LOG_INFO("非 OpenWRT 环境, 将尝试加载配置 %" PRIu8, g_prog_account);
    }
    else
    {
        LOG_INFO("非 OpenWRT 环境, 仅会尝试加载第一个有效配置");
    }

    for (uint8_t i = 0; i < cnt; i++)
    {
        /**
         * 指定了 --account 时只加载对应序号的配置,
         * 未指定时保持原有行为 (取第一个有效配置)
         */
        if (g_prog_account != 0 && i + 1 != g_prog_account)
        {
            continue;
        }

        const cJSON* account = cJSON_GetArrayItem(accounts, i);

        const cJSON* usr = cJSON_GetObjectItem(account, "username");
        const cJSON* pwd = cJSON_GetObjectItem(account, "password");
        const cJSON* chn = cJSON_GetObjectItem(account, "channel");
        const cJSON* time_windows_item = cJSON_GetObjectItem(account, "time_windows");

        // 检查账号
        if (usr == NULL)
        {
            LOG_WARN("配置 %" PRIu8 " username 参数不存在, 跳过当前配置", i + 1);
            continue;
        }
        if (usr->valuestring[0] == '\0')
        {
            LOG_WARN("配置 %" PRIu8 " username 参数为空, 跳过当前配置", i + 1);
            continue;
        }

        // 检查密码
        if (pwd == NULL)
        {
            LOG_WARN("配置 %" PRIu8 " password 参数不存在, 跳过当前配置", i + 1);
            continue;
        }
        if (pwd->valuestring[0] == '\0')
        {
            LOG_WARN("配置 %" PRIu8 " password 参数为空, 跳过当前配置", i + 1);
            continue;
        }

        // 检查时间控制字段
        if (apply_time_windows(time_windows_item, &g_prog_status[0].login_cfg) == false)
        {
            LOG_FATAL("配置 %" PRIu8 " time_windows 非法, 应为 [{ \"start\": \"mon 08:13\", \"end\": \"mon 23:57\" }, ...]", i + 1);
            cJSON_Delete(cfg_json);
            return false;
        }

        snprintf(g_prog_status[0].login_cfg.usr, USR_LEN, "%s", safe_str(usr->valuestring));
        snprintf(g_prog_status[0].login_cfg.pwd, PWD_LEN, "%s", safe_str(pwd->valuestring));

        g_prog_status[0].login_cfg.chn = parse_channel_json(chn, i + 1);
        apply_channel_ua(&g_prog_status[0].login_cfg, i + 1);

        LOG_DEBUG("使用 UA: %s", g_prog_status[0].login_cfg.user_agent);
        LOG_DEBUG("当前使用下标: 0");

        // 记真实序号: 首个配置可能不可用而被跳过, 写死 1 会让日志报错配置号
        g_prog_status[0].login_cfg.idx = i + 1;
        LOG_INFO("配置 %" PRIu8 " 可用, 将会尝试使用", i + 1);
        valid_cnt++;
        break;
    }

#endif

    cJSON_Delete(cfg_json);

    /**
     * 指定了 --account 时只保留该序号的配置
     *
     * 挑选必须放在全部配置加载完成之后:
     * 自动标记值是按"可用配置"的顺序算出来的 (0x100 + valid_i * 0x100),
     * 若在遍历时就跳过其它配置, valid_i 会从头计数, 标记值就会全部错位
     */
    if (g_prog_account != 0)
    {
        int8_t pick = -1;
        for (uint8_t i = 0; i < valid_cnt; i++)
        {
            if (g_prog_status[i].login_cfg.idx == g_prog_account)
            {
                pick = (int8_t)i;
                break;
            }
        }

        if (pick < 0)
        {
            LOG_FATAL("配置 %" PRIu8 " 不存在或该配置不可用, 请检查配置文件", g_prog_account);
            cfg_halt();
            return false;
        }

        if (pick != 0)
        {
            g_prog_status[0] = g_prog_status[pick];
        }

        valid_cnt = 1;

        // 单账号进程用不到其余配置的空间, 按实际情况回收
        prog_status_t* shrunk = realloc(g_prog_status, sizeof(prog_status_t));
        if (shrunk != NULL)
        {
            g_prog_status = shrunk;
        }

        LOG_INFO("仅加载配置 %" PRIu8 ", 标记值: 0x%x", g_prog_account, g_prog_status[0].login_cfg.mark);
    }

    if (valid_cnt == 0)
    {
        LOG_FATAL("无可用配置, 请检查后重启程序");
        cfg_halt();
        return false;
    }

    g_prog_cnt = valid_cnt;

    g_cfg_loaded = true;

    return true;
}

int list_accounts()
{
    /**
     * stdout 要留给账号列表, 日志一行都不落盘 (全部改写到 stderr).
     *
     * 查询模式必须在 init_logger / load_cfg 之前打开: 这两个调用本身就会写日志,
     * 而写进 run.log 的那几行会被 OpenWrt 的启动脚本当成上一轮运行留下的日志
     * 归档走 (见 set_logger_query_mode 的说明)
     */
    set_logger_query_mode(true);
    set_logger_console(false);

    if (init_logger() == false)
    {
        fprintf(stderr, "[ERROR] 日志系统初始化失败\n");
        return -1;
    }

    /**
     * 复用 load_cfg 的整套校验:
     * 列举出来的账号与真正会被加载的必须完全一致, 否则 init 脚本会起出跑不起来的实例
     */
    s_list_only = true;
    const bool loaded = load_cfg();
    s_list_only = false;

    if (loaded == false)
    {
        // 配置有问题时 load_cfg 已经把原因写到 stderr 了
        return -1;
    }

    for (uint8_t i = 0; i < g_prog_cnt; i++)
    {
        printf("%" PRIu8 "\n", g_prog_status[i].login_cfg.idx);
    }
    fflush(stdout);

    // 这里不调用 clean_logger: 它会把 run.log 改名收尾,
    // 而多实例下 run.log 是所有进程共用的, 每次列举都改名会破坏其它实例的写入
    return g_prog_cnt;
}

const char* print_log_dir()
{
    /**
     * stdout 要留给路径, 日志一行都不落盘 (全部改写到 stderr).
     *
     * 查询模式必须在 init_logger / load_cfg 之前打开。
     * 这个查询是 OpenWrt 启动脚本在归档上一轮日志【之前】调的, 而它自己会写三行:
     * 没有旧日志时这三行让 run.log 从"不存在"变成"非空", 脚本于是凭空归档出一个
     * 只有查询输出的 .log; 有旧日志时这三行会混进归档里 (见 set_logger_query_mode)
     */
    set_logger_query_mode(true);
    set_logger_console(false);

    if (init_logger() == false)
    {
        fprintf(stderr, "[ERROR] 日志系统初始化失败\n");
        return NULL;
    }

    /**
     * 日志目录由配置里的 log_dir 决定, 只有读完配置才知道到底在哪。
     * 配置有问题时按"列举账号"那套处理: 直接失败, 别挂住 —— 这个查询是给脚本调的
     */
    s_list_only = true;
    const bool loaded = load_cfg();
    s_list_only = false;

    if (loaded == false)
    {
        // 配置有问题时 load_cfg 已经把原因写到 stderr 了
        return NULL;
    }

    return get_logger_dir();
}
