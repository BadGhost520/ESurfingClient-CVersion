#include "webserver/HotspotControl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils/log/Logger.h"
#include "utils/platform/PlatformUtils.h"
#include "utils/runtime/ChildRun.h"
#include "utils/sim/SimThread.h"

#include <cJSON/cJSON.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

/** 状态查询最多等多久 */
#define HOTSPOT_STATUS_TIMEOUT_MS 10000
/** 开关动作最多等多久 (Windows 自己切换热点就可能要十几秒; nmcli 起 AP 也要几秒) */
#define HOTSPOT_ACTION_TIMEOUT_MS 30000
/** 后台多久刷新一次状态 */
#define HOTSPOT_REFRESH_MS 5000
/** 网页多久没来问过, 就不必再刷状态了 (没人看就不起子进程) */
#define HOTSPOT_REQUEST_WINDOW_MS 20000
/** 脚本/命令输出最多收多少 */
#define HOTSPOT_OUTPUT_MAX 8192

/* ------------------------------------------------------------------ 平台相关 */

/**
 * @brief 这个平台能不能控制热点
 */
static bool hotspot_platform_available(void);

/**
 * @brief 查一次热点状态
 * @return JSON 文本 (需要 free): {"adapters":[{"id","name","state","capability","connected"}],
 *         "defaultId":"","message":""}; 失败返回 NULL
 */
static char* hotspot_query_status(void);

/**
 * @brief 执行一次开关
 * @param action "start" / "stop"
 * @param adapter_id 目标网卡
 * @param message 返回给用户的结果说明 (需要 free)
 * @return 是否成功
 */
static bool hotspot_do_action(const char* action, const char* adapter_id, char** message);

#ifdef _WIN32

/* ------------------------------------------------------------------ Windows */

/** 内嵌脚本的调用方式: 变量前缀 + 脚本正文 -> UTF-16LE -> Base64 -> -EncodedCommand */
#define HOTSPOT_SCRIPT_MAX 8192
/** 1601-01-01 到 1970-01-01 的 100 纳秒数 (FILETIME 转 Unix 时间用) */
#define HOTSPOT_EPOCH_DIFF 116444736000000000ULL

#include "HotspotScript.inc"

/**
 * @brief 把 UTF-8 文本转成 UTF-16LE (PowerShell 的 -EncodedCommand 只吃这个)
 * @return 写入的字节数, 失败返回 0
 */
static size_t hotspot_utf8_to_utf16le(const char* text, unsigned char* out, const size_t out_len)
{
    if (text == NULL || out == NULL || out_len < 2) return 0;
    const int chars = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
    if (chars <= 1) return 0;
    if ((size_t)chars * 2 > out_len) return 0;
    if (MultiByteToWideChar(CP_UTF8, 0, text, -1, (wchar_t*)out, chars) <= 0) return 0;
    return (size_t)chars * 2 - 2; // 去掉结尾的 L'\0'
}

/**
 * @brief Base64 编码 (PowerShell -EncodedCommand 的格式)
 */
static bool hotspot_base64(const unsigned char* data, const size_t data_len, char* out, const size_t out_len)
{
    static const char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    const size_t need = (data_len + 2) / 3 * 4 + 1;
    if (out == NULL || out_len < need) return false;

    size_t used = 0;
    for (size_t i = 0; i < data_len; i += 3)
    {
        const unsigned int b0 = data[i];
        const unsigned int b1 = (i + 1 < data_len) ? data[i + 1] : 0;
        const unsigned int b2 = (i + 2 < data_len) ? data[i + 2] : 0;
        const unsigned int triple = (b0 << 16) | (b1 << 8) | b2;
        out[used++] = table[(triple >> 18) & 0x3F];
        out[used++] = table[(triple >> 12) & 0x3F];
        out[used++] = (i + 1 < data_len) ? table[(triple >> 6) & 0x3F] : '=';
        out[used++] = (i + 2 < data_len) ? table[triple & 0x3F] : '=';
    }
    out[used] = '\0';
    return true;
}

static bool hotspot_powershell(char* out, const size_t out_len)
{
    char system_root[PATH_MAX];
    if (GetEnvironmentVariableA("SystemRoot", system_root, (DWORD)sizeof(system_root)) == 0) return false;
    const int used = snprintf(out, out_len, "%s\\System32\\WindowsPowerShell\\v1.0\\powershell.exe", system_root);
    return used > 0 && (size_t)used < out_len;
}

/**
 * @brief 跑一次内嵌脚本, 把它的 JSON 收回来
 */
static bool hotspot_run_script(const char* action, const char* adapter_id, char* out, const size_t out_len, int* exit_code)
{
    char powershell[PATH_MAX];
    if (hotspot_powershell(powershell, sizeof(powershell)) == false)
    {
        LOG_WARN("找不到 powershell.exe, 热点控制不可用");
        return false;
    }

    char source[HOTSPOT_SCRIPT_MAX];
    const int used = snprintf(source, sizeof(source), "$EfAction = '%s'\n$EfAdapterId = '%s'\n%s",
                              action != NULL ? action : "state",
                              adapter_id != NULL ? adapter_id : "",
                              HOTSPOT_SCRIPT);
    if (used <= 0 || (size_t)used >= sizeof(source))
    {
        LOG_WARN("热点控制命令过长, 跳过");
        return false;
    }

    unsigned char utf16[HOTSPOT_SCRIPT_MAX * 2];
    const size_t utf16_len = hotspot_utf8_to_utf16le(source, utf16, sizeof(utf16));
    if (utf16_len == 0) { LOG_WARN("热点脚本 UTF-16 转换失败 (源码 %d 字节)", (int)strlen(source)); return false; }

    char encoded[HOTSPOT_SCRIPT_MAX * 3];
    if (hotspot_base64(utf16, utf16_len, encoded, sizeof(encoded)) == false) { LOG_WARN("热点脚本 base64 失败 (%d 字节)", (int)utf16_len); return false; }

    char args[HOTSPOT_SCRIPT_MAX * 3 + 128];
    const int arg_len = snprintf(args, sizeof(args),
                                 "-NoProfile -NonInteractive -ExecutionPolicy Bypass -EncodedCommand %s", encoded);
    if (arg_len <= 0 || (size_t)arg_len >= sizeof(args)) { LOG_WARN("热点命令行过长 (%d 字节)", (int)arg_len); return false; }

    const int timeout = (strcmp(action, "state") == 0) ? HOTSPOT_STATUS_TIMEOUT_MS : HOTSPOT_ACTION_TIMEOUT_MS;
    // 只要 stdout: PowerShell 会把进度/错误以 CLIXML 写到 stderr, 混进来会污染 JSON
    if (child_run_capture(powershell, args, out, out_len, timeout, exit_code, false) == false)
    {
        LOG_WARN("热点脚本执行失败 (退出码 %d)", exit_code != NULL ? *exit_code : -1);
        return false;
    }

    // 脚本只该打印一行 JSON, 但 PowerShell 偶尔会多打点警告: 掐出第一对花括号再来解析
    const char* begin = strchr(out, '{');
    char* end = strrchr(out, '}');
    if (begin == NULL || end == NULL || end < begin) return false;
    end[1] = '\0';
    if (begin != out) memmove(out, begin, strlen(begin) + 1);

    cJSON* json = cJSON_Parse(out);
    if (json == NULL)
    {
        LOG_WARN("热点脚本的输出无法解析 (退出码 %d): %.160s", exit_code != NULL ? *exit_code : -1, out);
        return false;
    }
    cJSON_Delete(json);
    return true;
}

static bool hotspot_platform_available(void)
{
    char powershell[PATH_MAX];
    if (hotspot_powershell(powershell, sizeof(powershell)) == false)
    {
        LOG_DEBUG("找不到 powershell.exe, 热点控制不可用");
        return false;
    }
    return true;
}

static char* hotspot_query_status(void)
{
    char output[HOTSPOT_OUTPUT_MAX];
    if (hotspot_run_script("state", NULL, output, sizeof(output), NULL) == false) return NULL;
    return strdup(output);
}

static bool hotspot_do_action(const char* action, const char* adapter_id, char** message)
{
    char output[HOTSPOT_OUTPUT_MAX];
    int exit_code = -1;
    if (hotspot_run_script(action, adapter_id, output, sizeof(output), &exit_code) == false)
    {
        *message = NULL;
        return false;
    }

    cJSON* json = cJSON_Parse(output);
    if (json == NULL) return false;

    const cJSON* text = cJSON_GetObjectItem(json, "message");
    if (cJSON_IsString(text) && text->valuestring != NULL) *message = strdup(text->valuestring);

    const cJSON* ok = cJSON_GetObjectItem(json, "ok");
    const bool success = cJSON_IsBool(ok) ? cJSON_IsTrue(ok) : (exit_code == 0);
    cJSON_Delete(json);
    return success;
}

#elif defined(__linux__) && !defined(__OPENWRT__)

/* ------------------------------------------------------------------ Linux (NetworkManager) */

/** 我们自己创建/使用的热点连接名 (SSID 同名) */
#define HOTSPOT_CONNECTION "ESurfingHotspot"

/**
 * @brief 跑一次 nmcli
 * @param args 参数 (不含 nmcli 本身)
 * @return 是否跑完 (退出码在 exit_code)
 */
static bool hotspot_nmcli(const char* args, char* out, const size_t out_len, int* exit_code)
{
    const bool ok = child_run_capture("nmcli", args, out, out_len, HOTSPOT_STATUS_TIMEOUT_MS, exit_code, false);
    if (ok == false) LOG_DEBUG("执行 nmcli 失败: %s", args);
    return ok;
}

/**
 * @brief 去掉输出末尾的换行
 */
static void hotspot_trim(char* text)
{
    size_t len = strlen(text);
    while (len > 0 && (text[len - 1] == '\n' || text[len - 1] == '\r')) text[--len] = '\0';
}

/**
 * @brief 网卡名只允许这些字符 (防止把别的东西拼进 nmcli 参数)
 */
static bool hotspot_valid_device(const char* name)
{
    if (name == NULL || name[0] == '\0' || strlen(name) > 32) return false;
    for (const char* p = name; *p != '\0'; p++)
    {
        const bool ok = (*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9') ||
                        *p == '_' || *p == '-' || *p == '.' || *p == ':';
        if (ok == false) return false;
    }
    return true;
}

/**
 * @brief 热点能力: NetworkManager 的 WIFI-PROPERTIES.AP
 */
static bool hotspot_ap_capable(const char* device, bool* capable)
{
    char args[512];
    char output[2048];
    int exit_code = -1;

    snprintf(args, sizeof(args), "-t -f WIFI-PROPERTIES.AP device show %s", device);
    if (hotspot_nmcli(args, output, sizeof(output), &exit_code) == false || exit_code != 0) return false;

    *capable = strstr(output, "WIFI-PROPERTIES.AP:yes") != NULL;
    return true;
}

/**
 * @brief 我们那条热点连接现在是不是在某块网卡上激活了
 */
static bool hotspot_profile_active(const char* device, bool* active)
{
    char output[4096];
    int exit_code = -1;

    *active = false;
    if (hotspot_nmcli("-t -f NAME,DEVICE connection show --active", output, sizeof(output), &exit_code) == false || exit_code != 0)
    {
        return false;
    }

    const size_t name_len = strlen(HOTSPOT_CONNECTION);
    const char* line = output;
    while (*line != '\0')
    {
        // 形如 "ESurfingHotspot:wlan0"
        if (strncmp(line, HOTSPOT_CONNECTION, name_len) == 0 && line[name_len] == ':')
        {
            const char* bound = line + name_len + 1;
            const char* end = strchr(bound, '\n');
            const size_t bound_len = end != NULL ? (size_t)(end - bound) : strlen(bound);
            if (device == NULL || (strlen(device) == bound_len && strncmp(bound, device, bound_len) == 0))
            {
                *active = true;
                return true;
            }
        }
        line = strchr(line, '\n');
        if (line == NULL) break;
        line++;
    }
    return true;
}

static bool hotspot_profile_exists(void)
{
    char output[4096];
    int exit_code = -1;
    if (hotspot_nmcli("-t -f NAME connection show", output, sizeof(output), &exit_code) == false || exit_code != 0) return false;

    const size_t name_len = strlen(HOTSPOT_CONNECTION);
    const char* line = output;
    while (*line != '\0')
    {
        if (strncmp(line, HOTSPOT_CONNECTION, name_len) == 0 && (line[name_len] == '\n' || line[name_len] == '\0')) return true;
        line = strchr(line, '\n');
        if (line == NULL) break;
        line++;
    }
    return false;
}

static bool hotspot_platform_available(void)
{
    char output[512];
    int exit_code = -1;
    if (child_run_capture("nmcli", "--version", output, sizeof(output), 3000, &exit_code, true) == false || exit_code != 0)
    {
        LOG_DEBUG("没找到 nmcli (NetworkManager), 热点控制不可用");
        return false;
    }
    return true;
}

/**
 * @brief 列出 wifi 网卡并给出状态 JSON
 *
 * nmcli -t 的输出用 ':' 分隔, 网卡名里不会出现冒号
 */
static char* hotspot_query_status(void)
{
    char devices[HOTSPOT_OUTPUT_MAX];
    int exit_code = -1;
    if (hotspot_nmcli("-t -f DEVICE,TYPE,STATE device", devices, sizeof(devices), &exit_code) == false || exit_code != 0)
    {
        return NULL;
    }

    cJSON* root = cJSON_CreateObject();
    cJSON* adapters = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "adapters", adapters);
    cJSON_AddStringToObject(root, "defaultId", "");
    cJSON_AddStringToObject(root, "message", "");

    const char* line = devices;
    while (line != NULL && *line != '\0')
    {
        char row[256];
        const char* end = strchr(line, '\n');
        const size_t len = end != NULL ? (size_t)(end - line) : strlen(line);
        if (len >= sizeof(row)) { line = end != NULL ? end + 1 : NULL; continue; }
        memcpy(row, line, len);
        row[len] = '\0';
        line = end != NULL ? end + 1 : NULL;

        char* type = strchr(row, ':');
        if (type == NULL) continue;
        *type++ = '\0';
        char* state = strchr(type, ':');
        if (state == NULL) continue;
        *state++ = '\0';
        if (strcmp(type, "wifi") != 0) continue; // 只有无线网卡能开热点

        bool capable = false;
        hotspot_ap_capable(row, &capable);
        bool active = false;
        hotspot_profile_active(row, &active);

        const bool transitioning = strstr(state, "activating") != NULL || strstr(state, "deactivating") != NULL;
        const bool usable = strcmp(state, "unavailable") != 0 && strcmp(state, "unmanaged") != 0;

        cJSON* item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "id", row);
        cJSON_AddStringToObject(item, "name", row);
        cJSON_AddStringToObject(item, "state", active ? "On" : (transitioning ? "InTransition" : "Off"));
        cJSON_AddStringToObject(item, "capability", capable ? "Enabled" : (usable ? "Disabled" : "Unknown"));
        cJSON_AddBoolToObject(item, "connected", usable);
        cJSON_AddItemToArray(adapters, item);
    }

    if (cJSON_GetArraySize(adapters) == 0)
    {
        cJSON_ReplaceItemInObject(root, "message", cJSON_CreateString("没有可用的无线网卡（需要支持 AP 模式的网卡与 NetworkManager）。"));
    }
    else
    {
        cJSON_ReplaceItemInObject(root, "message", cJSON_CreateString(""));
    }

    char* json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json;
}

static bool hotspot_do_action(const char* action, const char* adapter_id, char** message)
{
    *message = NULL;

    if (hotspot_valid_device(adapter_id) == false)
    {
        *message = strdup("网卡名不合法。");
        return false;
    }

    char args[1024];
    char output[HOTSPOT_OUTPUT_MAX];
    int exit_code = -1;

    if (strcmp(action, "start") == 0)
    {
        if (hotspot_profile_exists())
        {
            snprintf(args, sizeof(args), "connection up %s", HOTSPOT_CONNECTION);
        }
        else
        {
            // 首次开启: 让 NetworkManager 建一条名为 ESurfingHotspot 的共享连接 (它会自己配 NAT 与随机密码)
            snprintf(args, sizeof(args), "device wifi hotspot ifname %s con-name %s ssid %s",
                     adapter_id, HOTSPOT_CONNECTION, HOTSPOT_CONNECTION);
        }
    }
    else
    {
        snprintf(args, sizeof(args), "connection down %s", HOTSPOT_CONNECTION);
    }

    if (child_run_capture("nmcli", args, output, sizeof(output), HOTSPOT_ACTION_TIMEOUT_MS, &exit_code, true) == false)
    {
        *message = strdup("执行 nmcli 失败，请检查 NetworkManager 是否在运行。");
        return false;
    }

    hotspot_trim(output);
    if (exit_code != 0)
    {
        LOG_WARN("nmcli 执行失败 (退出码 %d): %s", exit_code, output);
        *message = output[0] != '\0' ? strdup(output) : strdup("热点操作失败，请检查 NetworkManager 权限与网卡状态。");
        return false;
    }

    if (strcmp(action, "start") == 0)
    {
        *message = strdup("热点已开启（SSID: ESurfingHotspot，密码在系统的网络设置里查看或修改）。");
    }
    else
    {
        *message = strdup("热点已关闭。");
    }
    return true;
}

#elif defined(__APPLE__)

/* ------------------------------------------------------------------ macOS (互联网共享) */

/**
 * macOS 的"互联网共享"没有受支持的命令行开关:
 * 状态可以查 launchd 里 com.apple.InternetSharing 这个 job 有没有在跑,
 * 开关只能尝试 launchctl (需要管理员权限, 没有免密 sudo 时会失败)。
 * 实验性功能, 没有在真机上验证过。
 */
#define HOTSPOT_MAC_PLIST "/System/Library/LaunchDaemons/com.apple.InternetSharing.plist"
#define HOTSPOT_MAC_LABEL "com.apple.InternetSharing"

/**
 * @brief 去掉外部命令输出末尾的换行
 */
static void hotspot_trim_platform(char* text)
{
    if (text == NULL) return;
    size_t len = strlen(text);
    while (len > 0 && (text[len - 1] == '\n' || text[len - 1] == '\r')) text[--len] = '\0';
}

/**
 * @brief 互联网共享现在是不是开着
 * @return 是否查询成功 (查询失败时 *running 为 false)
 */
static bool hotspot_mac_running(bool* running)
{
    char output[2048];
    int exit_code = -1;
    *running = false;

    if (child_run_capture("launchctl", "list " HOTSPOT_MAC_LABEL, output, sizeof(output), 5000, &exit_code, true) == false)
    {
        return false;
    }
    if (exit_code != 0) return true; // 没这个 job → 视为关闭
    // 运行中的 job 会带一行 "PID" = <数字>
    *running = strstr(output, "\"PID\"") != NULL;
    return true;
}

static bool hotspot_platform_available(void)
{
    // 有 InternetSharing 的 launchd 描述才算这个系统支持互联网共享
    return access(HOTSPOT_MAC_PLIST, R_OK) == 0;
}

/**
 * @brief 列出可以用来"共享出去"的网卡 (en* 这类真实网卡)
 */
static char* hotspot_query_status(void)
{
    bool running = false;
    hotspot_mac_running(&running);

    cJSON* root = cJSON_CreateObject();
    cJSON* adapters = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "adapters", adapters);
    cJSON_AddStringToObject(root, "defaultId", "");
    cJSON_AddBoolToObject(root, "experimental", true);

    struct ifaddrs* list = NULL;
    if (getifaddrs(&list) == 0)
    {
        for (struct ifaddrs* ifa = list; ifa != NULL; ifa = ifa->ifa_next)
        {
            if (ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_LINK) continue;
            if ((ifa->ifa_flags & IFF_LOOPBACK) != 0) continue;
            if ((ifa->ifa_flags & IFF_UP) == 0) continue;
            // 只留 en0/en1 这类真实网卡, 跳过 awdl/utun/bridge 之流
            if (strncmp(ifa->ifa_name, "en", 2) != 0) continue;

            cJSON* item = cJSON_CreateObject();
            cJSON_AddStringToObject(item, "id", ifa->ifa_name);
            cJSON_AddStringToObject(item, "name", ifa->ifa_name);
            cJSON_AddStringToObject(item, "state", running ? "On" : "Off");
            cJSON_AddStringToObject(item, "capability", "Enabled");
            cJSON_AddBoolToObject(item, "connected", (ifa->ifa_flags & IFF_RUNNING) != 0);
            cJSON_AddItemToArray(adapters, item);
        }
        freeifaddrs(list);
    }

    if (cJSON_GetArraySize(adapters) == 0)
    {
        cJSON_AddStringToObject(root, "message", "没有可用的无线网卡（实验性功能，未在真机验证）。");
    }
    else
    {
        cJSON_AddStringToObject(root, "message", "macOS 的热点通过「互联网共享」提供，为实验性功能（未在真机验证）。");
    }

    char* json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json;
}

static bool hotspot_do_action(const char* action, const char* adapter_id, char** message)
{
    (void)adapter_id; // macOS 的共享网卡在系统设置里选, 这里只控制总开关
    *message = NULL;

    char args[512];
    char output[HOTSPOT_OUTPUT_MAX];
    int exit_code = -1;

    if (strcmp(action, "start") == 0)
    {
        snprintf(args, sizeof(args), "-n launchctl load -w %s", HOTSPOT_MAC_PLIST);
    }
    else
    {
        snprintf(args, sizeof(args), "-n launchctl unload -w %s", HOTSPOT_MAC_PLIST);
    }

    // sudo -n: 没有免密授权时立刻失败, 不会卡在密码提示上
    const bool ran = child_run_capture("sudo", args, output, sizeof(output), HOTSPOT_ACTION_TIMEOUT_MS, &exit_code, true);
    hotspot_trim_platform(output);
    if (ran == false || exit_code != 0)
    {
        LOG_WARN("macOS 互联网共享开关失败 (退出码 %d): %s", exit_code, output);
        *message = strdup("需要管理员权限：请在「系统设置 → 通用 → 共享 → 互联网共享」里手动开启/关闭"
                          "（macOS 热点为实验性功能，未在真机验证）。");
        return false;
    }

    *message = strdup(strcmp(action, "start") == 0
                          ? "已请求开启互联网共享（实验性功能，请在系统设置里确认热点名称与密码）。"
                          : "已请求关闭互联网共享（实验性功能）。");
    return true;
}

#else

/* ------------------------------------------------------------------ 其它平台 */

static bool hotspot_platform_available(void)
{
    LOG_DEBUG("当前平台暂不支持热点控制");
    return false;
}

static char* hotspot_query_status(void)
{
    return NULL;
}

static bool hotspot_do_action(const char* action, const char* adapter_id, char** message)
{
    (void)action;
    (void)adapter_id;
    if (message != NULL) *message = NULL;
    return false;
}

#endif

/* ------------------------------------------------------------------ 平台无关的骨架 */

#ifdef _WIN32
typedef CRITICAL_SECTION hotspot_lock_t;
#else
#include <pthread.h>
typedef pthread_mutex_t hotspot_lock_t;
#endif

static hotspot_lock_t s_run_lock;   // 串行化"跑一次外部命令"
static hotspot_lock_t s_cache_lock; // 保护缓存与消息
static bool s_locks_ready;
static bool s_available_checked;
static bool s_available;

static char* s_status_cache;        // 最近一次状态 JSON
static char s_last_message[512];    // 最近一次开关动作的结果说明
static volatile bool s_busy;        // 正在切换
static volatile bool s_refreshing;  // 后台刷新线程是否该继续
static sim_thread_t* s_refresh_thread;
static uint64_t s_last_request_ms;
static uint64_t s_last_query_ms;

static void hotspot_lock_init(void)
{
#ifdef _WIN32
    InitializeCriticalSection(&s_run_lock);
    InitializeCriticalSection(&s_cache_lock);
#else
    pthread_mutex_init(&s_run_lock, NULL);
    pthread_mutex_init(&s_cache_lock, NULL);
#endif
    s_locks_ready = true;
}

static void hotspot_run_lock(void)
{
#ifdef _WIN32
    EnterCriticalSection(&s_run_lock);
#else
    pthread_mutex_lock(&s_run_lock);
#endif
}

static void hotspot_run_unlock(void)
{
#ifdef _WIN32
    LeaveCriticalSection(&s_run_lock);
#else
    pthread_mutex_unlock(&s_run_lock);
#endif
}

static void hotspot_cache_lock(void)
{
#ifdef _WIN32
    EnterCriticalSection(&s_cache_lock);
#else
    pthread_mutex_lock(&s_cache_lock);
#endif
}

static void hotspot_cache_unlock(void)
{
#ifdef _WIN32
    LeaveCriticalSection(&s_cache_lock);
#else
    pthread_mutex_unlock(&s_cache_lock);
#endif
}

static uint64_t hotspot_now_ms(void)
{
#ifdef _WIN32
    FILETIME file_time;
    GetSystemTimeAsFileTime(&file_time);
    ULARGE_INTEGER value;
    value.LowPart = file_time.dwLowDateTime;
    value.HighPart = file_time.dwHighDateTime;
    return (uint64_t)((value.QuadPart - HOTSPOT_EPOCH_DIFF) / 10000ULL);
#else
    struct timespec now;
    if (clock_gettime(CLOCK_REALTIME, &now) != 0) return 0;
    return (uint64_t)now.tv_sec * 1000ULL + (uint64_t)now.tv_nsec / 1000000ULL;
#endif
}

static void hotspot_sleep_ms(const int ms)
{
#ifdef _WIN32
    Sleep((DWORD)ms);
#else
    struct timespec delay;
    delay.tv_sec = ms / 1000;
    delay.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&delay, NULL);
#endif
}

static char* hotspot_strdup(const char* text)
{
    if (text == NULL) return NULL;
    const size_t len = strlen(text) + 1;
    char* copy = malloc(len);
    if (copy != NULL) memcpy(copy, text, len);
    return copy;
}

/**
 * @brief 刷新一次状态缓存
 */
static void hotspot_refresh_state(void)
{
    s_last_query_ms = hotspot_now_ms();
    hotspot_run_lock();
    char* fresh = hotspot_query_status();
    hotspot_run_unlock();
    if (fresh == NULL) return;

    hotspot_cache_lock();
    free(s_status_cache);
    s_status_cache = fresh;
    hotspot_cache_unlock();
}

typedef struct
{
    char action[8];
    char adapter_id[ADAPTER_ID_LEN];
} hotspot_job_t;

/**
 * @brief 开关工作线程: 跑一次动作, 记下结果说明, 然后刷新状态
 */
static int hotspot_worker(void* arg)
{
    hotspot_job_t* job = arg;
    char* message = NULL;
    bool ok;

    hotspot_run_lock();
    ok = hotspot_do_action(job->action, job->adapter_id, &message);
    hotspot_run_unlock();

    hotspot_cache_lock();
    s_last_message[0] = '\0';
    if (message != NULL && message[0] != '\0')
    {
        snprintf(s_last_message, sizeof(s_last_message), "%s", message);
    }
    else if (ok == false)
    {
        snprintf(s_last_message, sizeof(s_last_message), "热点操作未完成，请检查系统热点设置。");
    }
    free(message);
    s_busy = false;
    hotspot_cache_unlock();

    hotspot_refresh_state();
    free(job);
    return 0;
}

/**
 * @brief 后台状态刷新线程: 只在网页最近来问过的时候才跑外部命令
 */
static int hotspot_refresh_thread(void* arg)
{
    (void)arg;
    while (s_refreshing)
    {
        hotspot_sleep_ms(200);
        if (s_refreshing == false) return 0;
        if (s_busy) continue; // 正在切换, 状态由工作线程收尾时刷新
        const uint64_t now = hotspot_now_ms();
        if (now - s_last_request_ms > HOTSPOT_REQUEST_WINDOW_MS) continue; // 没人看网页就不查
        if (now - s_last_query_ms < HOTSPOT_REFRESH_MS) continue;          // 5 秒内不重复查
        hotspot_refresh_state();
    }
    return 0;
}

bool hotspot_control_available(void)
{
    if (s_available_checked) return s_available;
    s_available_checked = true;
    s_available = hotspot_platform_available();
    return s_available;
}

void hotspot_control_start(void)
{
    if (s_refreshing) return;
    if (hotspot_control_available() == false) return;

    if (s_locks_ready == false) hotspot_lock_init();
    s_refreshing = true;
    s_refresh_thread = sim_thread_create(hotspot_refresh_thread, NULL);
    if (s_refresh_thread == NULL)
    {
        s_refreshing = false;
        LOG_WARN("热点状态刷新线程创建失败");
        return;
    }
    LOG_INFO("热点控制已就绪");
}

void hotspot_control_stop(void)
{
    if (s_refresh_thread == NULL) return;
    s_refreshing = false;
    int code = 0;
    sim_thread_join(s_refresh_thread, &code);
    s_refresh_thread = NULL;

    hotspot_cache_lock();
    free(s_status_cache);
    s_status_cache = NULL;
    hotspot_cache_unlock();
}

char* hotspot_status_json(void)
{
    if (hotspot_control_available() == false) return NULL;

    s_last_request_ms = hotspot_now_ms();

    char* cached = NULL;
    char message[512];
    message[0] = '\0';
    hotspot_cache_lock();
    if (s_status_cache != NULL) cached = hotspot_strdup(s_status_cache);
    if (s_last_message[0] != '\0') snprintf(message, sizeof(message), "%s", s_last_message);
    hotspot_cache_unlock();

    if (cached == NULL)
    {
        const char* placeholder = "{\"adapters\":[],\"defaultId\":\"\",\"busy\":false,\"message\":\"正在读取热点状态…\"}";
        return hotspot_strdup(placeholder);
    }

    // 在平台给的状态上加两件事: 是否正在切换、最近一次动作的说明
    cJSON* root = cJSON_Parse(cached);
    free(cached);
    if (root == NULL) return NULL;

    // 平台自己给的状态说明 (例如"没有可用的无线网卡") 要留着, 只有动作结果优先于它
    cJSON* platform_message = cJSON_DetachItemFromObject(root, "message");
    const char* platform_text = (cJSON_IsString(platform_message) && platform_message->valuestring != NULL)
                                    ? platform_message->valuestring
                                    : "";
    cJSON_DeleteItemFromObject(root, "busy");
    cJSON_AddBoolToObject(root, "busy", s_busy);

    const char* final_message = "";
    if (s_busy) final_message = "正在切换热点，请稍候。";
    else if (message[0] != '\0') final_message = message;
    else if (platform_text[0] != '\0') final_message = platform_text;
    cJSON_AddStringToObject(root, "message", final_message);
    cJSON_Delete(platform_message);
    char* json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json;
}

int hotspot_request(const char* action, const char* adapter_id, char** out_json)
{
    *out_json = NULL;
    if (hotspot_control_available() == false) return 501;
    if (action == NULL || (strcmp(action, "start") != 0 && strcmp(action, "stop") != 0)) return 400;
    if (adapter_id == NULL || adapter_id[0] == '\0') return 400;
    // 网卡名会被拼进 nmcli 参数: 只允许字母数字与 _ - . :, 长度也限住
    if (strlen(adapter_id) > 64 || strspn(adapter_id, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-.:") != strlen(adapter_id))
    {
        LOG_WARN("热点请求的网卡名不合法, 已拒绝: %.64s", adapter_id);
        return 400;
    }

    hotspot_cache_lock();
    if (s_busy)
    {
        hotspot_cache_unlock();
        *out_json = hotspot_strdup("{\"ok\":false,\"message\":\"正在切换热点，请稍候。\"}");
        return 409;
    }
    s_busy = true;
    s_last_message[0] = '\0';
    hotspot_cache_unlock();

    hotspot_job_t* job = calloc(1, sizeof(hotspot_job_t));
    if (job == NULL)
    {
        s_busy = false;
        return 500;
    }
    snprintf(job->action, sizeof(job->action), "%s", action);
    snprintf(job->adapter_id, sizeof(job->adapter_id), "%s", adapter_id);

    sim_thread_t* worker = sim_thread_create(hotspot_worker, job);
    if (worker == NULL)
    {
        free(job);
        s_busy = false;
        return 500;
    }
    sim_thread_detach(worker); // 跑完自己收尾, 关闭程序时不等它

    LOG_INFO("收到热点%s请求 (网卡 %s), 已交给后台执行", strcmp(action, "start") == 0 ? "开启" : "关闭", adapter_id);
    *out_json = hotspot_strdup("{\"ok\":true,\"message\":\"正在切换热点，请稍候。\"}");
    return 202;
}
