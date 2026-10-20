#include "webserver/HotspotControl.h"

#include "utils/log/Logger.h"
#include "utils/platform/PlatformUtils.h"

#ifdef _WIN32

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cJSON/cJSON.h>

#include "utils/runtime/ChildRun.h"
#include "utils/sim/SimThread.h"

/* >>> 内嵌脚本 开始 (改脚本请改这里的字面量, 每行末尾的 \n 不能省) */
static const char HOTSPOT_SCRIPT[] =
        "$ErrorActionPreference = 'Stop'\n"
        "# 进度流在重定向时会以 CLIXML 写到 stderr, 会和 JSON 混在一起, 直接关掉\n"
        "$ProgressPreference = 'SilentlyContinue'\n"
        "\n"
        "# 主程序会收走 stdout 解析 JSON, 所以这里只打印一行 JSON;\n"
        "# 输出被重定向时统一 UTF-8, 否则中文到了日志/解析那里会变成问号\n"
        "if ([Console]::IsOutputRedirected) { [Console]::OutputEncoding = [Text.Encoding]::UTF8 }\n"
        "\n"
        "function Send-Result($Data, [int]$Code = 0) {\n"
        "    [Console]::Out.WriteLine(($Data | ConvertTo-Json -Depth 4 -Compress))\n"
        "    [Console]::Out.Flush()\n"
        "    if ($Code -ne 0) { exit $Code }\n"
        "}\n"
        "\n"
        "function Fail([string]$Message) {\n"
        "    Send-Result @{ ok = $false; message = $Message } 1\n"
        "}\n"
        "\n"
        "# 一次执行: 列网卡与热点状态, 或者开关一次热点。常驻监听/令牌/来源校验都不需要的 ——\n"
        "# 页面直接请求主程序自己的 /api/hotspot/*, 由主程序决定什么时候调用本脚本\n"
        "try {\n"
        "    if ($PSVersionTable.PSEdition -ne 'Desktop') { Fail '热点控制需要 Windows PowerShell 5.1。' }\n"
        "    [Windows.Networking.NetworkOperators.NetworkOperatorTetheringManager, Windows.Networking.NetworkOperators, ContentType = WindowsRuntime] > $null\n"
        "    [Windows.Networking.Connectivity.NetworkInformation, Windows.Networking.Connectivity, ContentType = WindowsRuntime] > $null\n"
        "\n"
        "    # 只有\"已连接且有连接配置\"的网卡才在系统热点设置的可选范围里\n"
        "    $profiles = @{}\n"
        "    foreach ($profile in [Windows.Networking.Connectivity.NetworkInformation]::GetConnectionProfiles()) {\n"
        "        if ($profile.NetworkAdapter) { $profiles[$profile.NetworkAdapter.NetworkAdapterId.ToString()] = $profile }\n"
        "    }\n"
        "\n"
        "    $rows = @()\n"
        "    $managers = @{}\n"
        "    foreach ($adapter in [Net.NetworkInformation.NetworkInterface]::GetAllNetworkInterfaces()) {\n"
        "        $id = $adapter.Id.Trim('{}').ToLowerInvariant()\n"
        "        if ($adapter.OperationalStatus -ne 'Up' -or -not $profiles.ContainsKey($id)) { continue }\n"
        "        try {\n"
        "            $profile = $profiles[$id]\n"
        "            $manager = [Windows.Networking.NetworkOperators.NetworkOperatorTetheringManager]::CreateFromConnectionProfile($profile)\n"
        "            $capability = [string][Windows.Networking.NetworkOperators.NetworkOperatorTetheringManager]::GetTetheringCapabilityFromConnectionProfile($profile)\n"
        "            $managers[$id] = $manager\n"
        "            $rows += [pscustomobject]@{\n"
        "                id         = $id\n"
        "                # 有些机器上连接名是空的 (例如只填了描述), 用描述兜底\n"
        "                name       = $(if ($adapter.Name) { $adapter.Name } else { $adapter.Description })\n"
        "                state      = [string]$manager.TetheringOperationalState\n"
        "                capability = $capability\n"
        "                connected  = $true\n"
        "            }\n"
        "        } catch { } # 单块网卡不支持热点不影响其它网卡\n"
        "    }\n"
        "\n"
        "    if ($EfAction -eq 'state') {\n"
        "        $internet = [Windows.Networking.Connectivity.NetworkInformation]::GetInternetConnectionProfile()\n"
        "        $default = ''\n"
        "        if ($internet -and $internet.NetworkAdapter) {\n"
        "            $default = $internet.NetworkAdapter.NetworkAdapterId.ToString().Trim('{}').ToLowerInvariant()\n"
        "        }\n"
        "        $busy = @($rows | Where-Object { $_.state -eq 'InTransition' }).Count -gt 0\n"
        "        $message = if ($rows.Count -eq 0) { '没有已连接的网卡，热点控制不可用。' }\n"
        "                   elseif ($busy) { 'Windows 正在切换热点，请稍候。' }\n"
        "                   else { '' }\n"
        "        Send-Result @{ adapters = $rows; defaultId = $default; busy = $busy; message = $message }\n"
        "        exit 0\n"
        "    }\n"
        "\n"
        "    if (-not $EfAdapterId) { Fail '没有指定网卡。' }\n"
        "    $target = $EfAdapterId.Trim('{}').ToLowerInvariant()\n"
        "    if (-not $managers.ContainsKey($target)) { Fail '所选网卡当前不可用，请刷新页面后重试。' }\n"
        "\n"
        "    $manager = $managers[$target]\n"
        "    $current = [string]$manager.TetheringOperationalState\n"
        "    if ($current -eq 'InTransition') { Fail 'Windows 正在切换热点，请稍后再试。' }\n"
        "\n"
        "    if ($EfAction -eq 'start') {\n"
        "        if ($current -eq 'On') { Send-Result @{ ok = $true; message = '热点已经是开启状态。'; state = $current }; exit 0 }\n"
        "        $operation = $manager.StartTetheringAsync()\n"
        "        $wanted = 'On'\n"
        "    }\n"
        "    else {\n"
        "        if ($current -eq 'Off') { Send-Result @{ ok = $true; message = '热点已经是关闭状态。'; state = $current }; exit 0 }\n"
        "        $operation = $manager.StopTetheringAsync()\n"
        "        $wanted = 'Off'\n"
        "    }\n"
        "\n"
        "    # 等状态真的变过来再回话 (Windows 切换热点常见几秒, 最多等 20 秒)\n"
        "    $state = $current\n"
        "    for ($i = 0; $i -lt 200; $i++) {\n"
        "        Start-Sleep -Milliseconds 100\n"
        "        $state = [string]$manager.TetheringOperationalState\n"
        "        if ($state -eq $wanted) { break }\n"
        "        if ($wanted -eq 'On' -and $state -eq 'Off' -and $i -gt 50) { break } # 明显起不来就早点收手\n"
        "    }\n"
        "    $null = $operation\n"
        "\n"
        "    if ($state -eq $wanted) {\n"
        "        $message = if ($wanted -eq 'On') { '热点已开启，手机可以连接。' } else { '热点已关闭。' }\n"
        "        Send-Result @{ ok = $true; message = $message; state = $state }\n"
        "        exit 0\n"
        "    }\n"
        "    Fail ('热点没有切换成功（当前状态：' + $state + '），请打开 Windows 热点设置检查。')\n"
        "}\n"
        "catch {\n"
        "    Fail ('热点操作失败：' + $_.Exception.GetBaseException().Message)\n"
        "}\n"
        "\n"
    ;
/* <<< 内嵌脚本 结束 */

/* 脚本已经内嵌在上面, 调用方式见 hotspot_run() */

/** 内嵌脚本拼装后的上限 (脚本约 5KB, 留足余量) */
#define HOTSPOT_SCRIPT_MAX 8192

/** 状态查询最多等多久 */
#define HOTSPOT_STATUS_TIMEOUT_MS 10000
/** 开关动作最多等多久 (Windows 自己切换热点就可能要十几秒) */
#define HOTSPOT_ACTION_TIMEOUT_MS 30000
/** 后台多久刷新一次状态 */
#define HOTSPOT_REFRESH_MS 5000
/** 网页多久没来问过, 就不必再刷状态了 (没人看就不起 PowerShell) */
#define HOTSPOT_REQUEST_WINDOW_MS 20000
/** 脚本输出最多收多少 */
#define HOTSPOT_OUTPUT_MAX 8192

static CRITICAL_SECTION s_run_lock;   // 串行化"跑一次 PowerShell"
static CRITICAL_SECTION s_cache_lock; // 保护缓存与消息
static bool s_locks_ready;
static bool s_available_checked;
static bool s_available;

static char* s_status_cache;          // 脚本吐出来的状态 JSON
static char s_last_message[512];      // 最近一次开关动作的结果说明
static volatile bool s_busy;          // 正在切换
static volatile bool s_refreshing;    // 后台刷新线程是否该继续
static sim_thread_t* s_refresh_thread;
static uint64_t s_last_request_ms;

/**
 * @brief 简单的字符串拷贝 (项目里没引入 strdup, 各编译器的名字还不一样)
 */
static char* hotspot_strdup(const char* text)
{
    if (text == NULL) return NULL;
    const size_t len = strlen(text) + 1;
    char* copy = malloc(len);
    if (copy != NULL) memcpy(copy, text, len);
    return copy;
}

static uint64_t hotspot_now_ms(void)
{
    FILETIME file_time;
    GetSystemTimeAsFileTime(&file_time);
    ULARGE_INTEGER value;
    value.LowPart = file_time.dwLowDateTime;
    value.HighPart = file_time.dwHighDateTime;
    return (uint64_t)((value.QuadPart - 116444736000000000ULL) / 10000ULL);
}

/**
 * @brief 把 UTF-8 文本转成 UTF-16LE (PowerShell 的 -EncodedCommand 只吃这个)
 * @param out 输出缓冲
 * @param out_len 缓冲字节数
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
 * @return 是否成功
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
 * @brief 跑一次 HotspotControl.ps1
 * @return 是否跑完并且拿到了 JSON
 */
static bool hotspot_run(const char* action, const char* adapter_id, char* out, const size_t out_len, int* exit_code)
{
    char powershell[PATH_MAX];
    if (hotspot_powershell(powershell, sizeof(powershell)) == false)
    {
        LOG_WARN("找不到 powershell.exe, 热点控制不可用");
        return false;
    }

    /**
     * 脚本已经嵌在程序里, 不再依赖发布包里的 .ps1:
     * 动作与网卡 id 以变量形式放在脚本前面, 整段用 -EncodedCommand 交给 PowerShell
     * (参数里只会出现 state/start/stop 与 GUID, 不会把用户输入拼进代码)
     */
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

/**
 * @brief 刷新一次状态缓存
 */
static void hotspot_refresh_state(void)
{
    char output[HOTSPOT_OUTPUT_MAX];
    EnterCriticalSection(&s_run_lock);
    const bool ok = hotspot_run("state", NULL, output, sizeof(output), NULL);
    LeaveCriticalSection(&s_run_lock);
    if (ok == false) return;

    EnterCriticalSection(&s_cache_lock);
    free(s_status_cache);
    s_status_cache = hotspot_strdup(output);
    LeaveCriticalSection(&s_cache_lock);
}

typedef struct
{
    char action[8];
    char adapter_id[ADAPTER_ID_LEN];
} hotspot_job_t;

/**
 * @brief 开关工作线程: 跑一次 start/stop, 然后立刻刷新状态
 */
static int hotspot_worker(void* arg)
{
    hotspot_job_t* job = arg;
    char output[HOTSPOT_OUTPUT_MAX];
    int exit_code = -1;
    bool ok = false;

    EnterCriticalSection(&s_run_lock);
    ok = hotspot_run(job->action, job->adapter_id, output, sizeof(output), &exit_code);
    LeaveCriticalSection(&s_run_lock);

    EnterCriticalSection(&s_cache_lock);
    s_last_message[0] = '\0';
    if (ok)
    {
        cJSON* json = cJSON_Parse(output);
        if (json != NULL)
        {
            const cJSON* message = cJSON_GetObjectItem(json, "message");
            const cJSON* success = cJSON_GetObjectItem(json, "ok");
            if (cJSON_IsString(message) && message->valuestring != NULL)
            {
                snprintf(s_last_message, sizeof(s_last_message), "%s", message->valuestring);
            }
            if (cJSON_IsBool(success) && cJSON_IsFalse(success))
            {
                ok = false;
            }
            cJSON_Delete(json);
        }
    }
    if (ok == false && s_last_message[0] == '\0')
    {
        snprintf(s_last_message, sizeof(s_last_message), "热点操作未完成，请打开 Windows 热点设置检查。");
    }
    s_busy = false;
    LeaveCriticalSection(&s_cache_lock);

    hotspot_refresh_state();
    free(job);
    return 0;
}

/**
 * @brief 后台状态刷新线程: 只在网页最近来问过的时候才起 PowerShell
 */
static int hotspot_refresh_thread(void* arg)
{
    (void)arg;
    while (s_refreshing)
    {
        for (int i = 0; i < HOTSPOT_REFRESH_MS / 200; i++)
        {
            if (s_refreshing == false) return 0;
            Sleep(200);
        }
        if (s_busy) continue; // 正在切换, 状态由工作线程收尾时刷新
        if (hotspot_now_ms() - s_last_request_ms > HOTSPOT_REQUEST_WINDOW_MS) continue;
        hotspot_refresh_state();
    }
    return 0;
}

bool hotspot_control_available(void)
{
    if (s_available_checked) return s_available;
    s_available_checked = true;
    s_available = false;

    // 脚本已经内嵌, 只要系统里有 Windows PowerShell 5.1 就能用
    char powershell[PATH_MAX];
    if (hotspot_powershell(powershell, sizeof(powershell)) == false)
    {
        LOG_DEBUG("找不到 powershell.exe, 热点控制不可用 (其它平台暂不支持)");
        return false;
    }
    s_available = true;
    return true;
}

void hotspot_control_start(void)
{
    if (s_refreshing) return;
    if (hotspot_control_available() == false) return;

    if (s_locks_ready == false)
    {
        InitializeCriticalSection(&s_run_lock);
        InitializeCriticalSection(&s_cache_lock);
        s_locks_ready = true;
    }
    s_refreshing = true;
    s_refresh_thread = sim_thread_create(hotspot_refresh_thread, NULL);
    if (s_refresh_thread == NULL)
    {
        s_refreshing = false;
        LOG_WARN("热点状态刷新线程创建失败");
        return;
    }
    LOG_INFO("热点控制已就绪 (内嵌脚本, 按需调用 PowerShell)");
}

void hotspot_control_stop(void)
{
    if (s_refresh_thread == NULL) return;
    s_refreshing = false;
    int code = 0;
    sim_thread_join(s_refresh_thread, &code);
    s_refresh_thread = NULL;

    EnterCriticalSection(&s_cache_lock);
    free(s_status_cache);
    s_status_cache = NULL;
    LeaveCriticalSection(&s_cache_lock);
}

char* hotspot_status_json(void)
{
    if (hotspot_control_available() == false) return NULL;

    s_last_request_ms = hotspot_now_ms();

    char* cached = NULL;
    char message[512];
    message[0] = '\0';
    EnterCriticalSection(&s_cache_lock);
    if (s_status_cache != NULL) cached = hotspot_strdup(s_status_cache);
    if (s_last_message[0] != '\0') snprintf(message, sizeof(message), "%s", s_last_message);
    LeaveCriticalSection(&s_cache_lock);

    if (cached == NULL)
    {
        const char* placeholder = "{\"adapters\":[],\"defaultId\":\"\",\"busy\":false,\"message\":\"正在读取热点状态…\"}";
        return hotspot_strdup(placeholder);
    }

    // 在脚本给的状态上加两件事: 是否正在切换、最近一次动作的说明
    cJSON* root = cJSON_Parse(cached);
    free(cached);
    if (root == NULL) return NULL;
    cJSON_DeleteItemFromObject(root, "busy");
    cJSON_DeleteItemFromObject(root, "message");
    cJSON_AddBoolToObject(root, "busy", s_busy);
    if (message[0] != '\0')
    {
        cJSON_AddStringToObject(root, "message", message);
    }
    else
    {
        cJSON_AddStringToObject(root, "message", s_busy ? "正在切换热点，请稍候。" : "");
    }
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

    EnterCriticalSection(&s_cache_lock);
    if (s_busy)
    {
        LeaveCriticalSection(&s_cache_lock);
        *out_json = hotspot_strdup("{\"ok\":false,\"message\":\"Windows 正在切换热点，请稍候。\"}");
        return 409;
    }
    s_busy = true;
    s_last_message[0] = '\0';
    LeaveCriticalSection(&s_cache_lock);

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
    sim_thread_detach(worker); // 跑完自己收尾, 关闭程序时不等它 (PowerShell 子进程会自己结束)

    LOG_INFO("收到热点%s请求 (网卡 %s), 已交给后台执行", strcmp(action, "start") == 0 ? "开启" : "关闭", adapter_id);
    *out_json = hotspot_strdup("{\"ok\":true,\"message\":\"Windows 正在切换热点，请稍候。\"}");
    return 202;
}

#else

bool hotspot_control_available(void)
{
    return false;
}

void hotspot_control_start(void)
{
}

void hotspot_control_stop(void)
{
}

char* hotspot_status_json(void)
{
    return NULL;
}

int hotspot_request(const char* action, const char* adapter_id, char** out_json)
{
    (void)action;
    (void)adapter_id;
    if (out_json != NULL) *out_json = NULL;
    return 501;
}

#endif
