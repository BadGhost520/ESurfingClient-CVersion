#include "webserver/WebServer.h"
#include "webserver/mongoose.h"

#include "control/Control.h"

#include "utils/sim/SimThread.h"

#include "utils/PlatformUtils.h"
#include "utils/Logger.h"
#include "utils/cJSON.h"

#include "NetClient.h"
#include "States.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifndef _WIN32
#include <strings.h>
#include <dirent.h>
#endif

static sim_thread_t* web_thread;

/**
 * @brief 认证状态是否就在本进程里
 *
 * - 单进程模式: Web 服务与认证逻辑同进程, 直接读 g_prog_status
 * - Web 进程 (--role web): 认证状态在另一个进程里, 通过控制通道查询
 *
 * 只有"运行时状态"要跨进程; 配置两个进程各读各的文件, 因此
 * /api/getConfigs 之类的接口两种模式下走的是同一段代码
 */
static bool s_local_state = true;

/**
 * @brief 查询认证状态
 * @param out 是否已认证
 * @return 是否查询成功 (Web 进程连不上认证进程时为 false)
 */
static bool query_is_authed(bool* out)
{
    if (s_local_state)
    {
        if (g_prog_status == NULL || g_prog_cnt <= 0) return false;
        *out = g_prog_status[0].runtime_status.is_authed;
        return true;
    }

    control_status_t status;
    if (control_query_status(&status) == false) return false;

    *out = status.is_authed;
    return true;
}

/**
 * @brief 请求重新认证
 * @return 是否成功
 */
static bool request_restart_auth()
{
    if (s_local_state)
    {
        if (g_prog_status == NULL || g_prog_cnt <= 0) return false;
        g_prog_status[0].runtime_status.is_need_reauth = true;
        return true;
    }

    return control_restart_auth();
}

/**
 * @brief 请求重新加载配置并重新认证
 * @return 是否成功
 */
static bool request_apply_config()
{
    if (s_local_state)
    {
        if (g_prog_status == NULL || g_prog_cnt <= 0) return false;
        g_cfg_loaded = false;
        g_prog_status[0].runtime_status.is_need_reauth = true;
        return true;
    }

    return control_apply_config();
}

/** @brief 日志文件列表最多返回的数量 */
#define LOG_FILE_MAX 64
/** @brief 单次读取日志文件的最大字节数 (超出时只返回末尾部分) */
#define LOG_READ_MAX (256 * 1024)
/** @brief 日志文件名缓冲区长度 */
#define LOG_NAME_LEN 256

/** @brief 日志文件的固定名字 (Logger.c s_file_name) */
static const char log_current_name[] = "run.log";

/** @brief 日志文件的后缀 (run.log / <时间戳>.log / <时间戳>-pid-seq.rotate.log 都以此结尾) */
static const char log_suffix[] = ".log";

/** @brief 通用 API 响应头 */
#define HEADER_JSON "Content-Type: application/json\r\nCache-Control: no-store\r\n"
#define HEADER_TEXT "Content-Type: text/plain; charset=utf-8\r\nCache-Control: no-store\r\n"
#define HEADER_TEXT_TRUNCATED HEADER_TEXT "X-Log-Truncated: 1\r\n"

/** @brief 日志文件信息 */
typedef struct
{
    char name[LOG_NAME_LEN];
    uint64_t size;
    uint64_t mtime;
    bool current;
} log_file_entry_t;

static const char* week_day_to_str(const int day)
{
    static const char* names[] = {"sun", "mon", "tue", "wed", "thu", "fri", "sat"};
    if (day < 0 || day > 6) return "sun";
    return names[day];
}

static void format_week_min(const uint16_t week_min, char* out)
{
    const uint16_t mod = week_min % WEEK_MINUTES;
    const int day = mod / 1440;
    const int hour = (mod % 1440) / 60;
    const int minute = mod % 60;
    snprintf(out, TIME_WINDOW_STR_LEN, "%s %02d:%02d", week_day_to_str(day), hour, minute);
}

static int str_case_cmp(const char* a, const char* b)
{
#ifdef _WIN32
    return _stricmp(a, b);
#else
    return strcasecmp(a, b);
#endif
}

#ifdef _WIN32
/**
 * @brief 通过文件句柄获取文件大小与修改时间
 * @note FindFirstFile 读取的是目录项, 正在被写入的日志文件 (run.log) 的大小与时间
 *       可能还没有同步到目录项, 这里用句柄再取一次真实值
 * @return 是否获取成功
 */
static bool win_stat_file(const char* path, uint64_t* size, uint64_t* mtime)
{
    HANDLE handle = CreateFileA(path, GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (handle == INVALID_HANDLE_VALUE) return false;

    LARGE_INTEGER file_size;
    BY_HANDLE_FILE_INFORMATION info;
    const bool ok = GetFileSizeEx(handle, &file_size) != 0 &&
        GetFileInformationByHandle(handle, &info) != 0;
    CloseHandle(handle);
    if (ok == false) return false;

    if (size != NULL) *size = (uint64_t)file_size.QuadPart;
    if (mtime != NULL)
    {
        const uint64_t file_time = ((uint64_t)info.ftLastWriteTime.dwHighDateTime << 32) |
            (uint64_t)info.ftLastWriteTime.dwLowDateTime;
        *mtime = file_time / 10000000ULL - 11644473600ULL;
    }
    return true;
}
#endif

/**
 * @brief 检查日志文件名是否安全 (禁止路径穿越)
 * @param name 文件名
 * @return 是否安全
 */
static bool is_safe_log_name(const char* name)
{
    if (name == NULL) return false;
    const size_t len = strlen(name);
    if (len == 0 || len >= LOG_NAME_LEN) return false;
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) return false;
    for (size_t i = 0; i < len; i++)
    {
        const unsigned char c = (unsigned char)name[i];
        if (isalnum(c) == 0 && c != '.' && c != '_' && c != '-') return false;
    }
    return true;
}

/**
 * @brief 检查文件名是不是日志文件
 *
 * 必须再认一次后缀: 日志目录默认就是【程序所在目录】(配置里的 log_dir),
 * 那里还躺着程序本体 / 配置文件 / portal, 只判"名字安全"的话
 * 日志页会把这些都列成日志, 连明文账号密码所在的 ESurfingClient.json
 * 都能当成日志读出来
 * @param name 文件名
 * @return 是否是日志文件
 */
static bool is_log_file_name(const char* name)
{
    if (is_safe_log_name(name) == false) return false;

    const size_t name_len = strlen(name);
    const size_t suffix_len = sizeof(log_suffix) - 1;
    if (name_len <= suffix_len) return false;

    return str_case_cmp(name + name_len - suffix_len, log_suffix) == 0;
}

/**
 * @brief 读取日志目录中的文件列表
 * @param out 输出数组
 * @param max 数组容量
 * @return 文件数量
 */
static int list_log_files(log_file_entry_t* out, const int max)
{
    const char* dir = get_logger_dir();
    if (dir == NULL || dir[0] == '\0') return 0;

    int count = 0;

#ifdef _WIN32

    char pattern[PATH_MAX];
    const int pattern_len = snprintf(pattern, sizeof(pattern), "%s%c*", dir, SEP);
    if (pattern_len <= 0 || (size_t)pattern_len >= sizeof(pattern)) return 0;

    WIN32_FIND_DATAA find_data;
    HANDLE handle = FindFirstFileA(pattern, &find_data);
    if (handle == INVALID_HANDLE_VALUE) return 0;
    do
    {
        if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) continue;
        if (count >= max) break;
        if (is_log_file_name(find_data.cFileName) == false) continue;

        snprintf(out[count].name, sizeof(out[count].name), "%s", find_data.cFileName);
        out[count].size = ((uint64_t)find_data.nFileSizeHigh << 32) | (uint64_t)find_data.nFileSizeLow;
        // FILETIME (100 纳秒, 1601 起) -> Unix 时间戳 (秒, 1970 起)
        const uint64_t file_time = ((uint64_t)find_data.ftLastWriteTime.dwHighDateTime << 32) |
            (uint64_t)find_data.ftLastWriteTime.dwLowDateTime;
        out[count].mtime = file_time / 10000000ULL - 11644473600ULL;
        out[count].current = strcmp(find_data.cFileName, log_current_name) == 0;

        char path[PATH_MAX];
        const int path_len = snprintf(path, sizeof(path), "%s%c%s", dir, SEP, find_data.cFileName);
        if (path_len > 0 && (size_t)path_len < sizeof(path))
        {
            win_stat_file(path, &out[count].size, &out[count].mtime);
        }
        count++;
    } while (FindNextFileA(handle, &find_data) != 0);
    FindClose(handle);

#else

    DIR* dir_handle = opendir(dir);
    if (dir_handle == NULL) return 0;

    char path[PATH_MAX];
    struct dirent* entry;
    while ((entry = readdir(dir_handle)) != NULL)
    {
        if (count >= max) break;
        if (is_log_file_name(entry->d_name) == false) continue;

        const int path_len = snprintf(path, sizeof(path), "%s%c%s", dir, SEP, entry->d_name);
        if (path_len <= 0 || (size_t)path_len >= sizeof(path)) continue;

        struct stat st;
        if (stat(path, &st) != 0 || !S_ISREG(st.st_mode)) continue;

        snprintf(out[count].name, sizeof(out[count].name), "%s", entry->d_name);
        out[count].size = (uint64_t)st.st_size;
        out[count].mtime = (uint64_t)st.st_mtime;
        out[count].current = strcmp(entry->d_name, log_current_name) == 0;
        count++;
    }
    closedir(dir_handle);

#endif

    return count;
}

/** @brief 排序: 当前日志在最前, 其余按修改时间倒序 */
static int cmp_log_files(const void* a, const void* b)
{
    const log_file_entry_t* left = (const log_file_entry_t*)a;
    const log_file_entry_t* right = (const log_file_entry_t*)b;
    if (left->current != right->current) return left->current ? -1 : 1;
    if (left->mtime != right->mtime) return left->mtime > right->mtime ? -1 : 1;
    return -str_case_cmp(left->name, right->name);
}

/**
 * @brief 读取日志文件内容
 * @param name 日志文件名
 * @param out 输出内容 (需要 free)
 * @param out_len 输出内容长度
 * @param truncated 是否被截断
 * @return 是否读取成功
 */static bool read_log_file(const char* name, char** out, size_t* out_len, bool* truncated)
{
    const char* dir = get_logger_dir();
    if (dir == NULL || dir[0] == '\0') return false;
    if (is_log_file_name(name) == false) return false;

    char path[PATH_MAX];
    const int path_len = snprintf(path, sizeof(path), "%s%c%s", dir, SEP, name);
    if (path_len <= 0 || (size_t)path_len >= sizeof(path)) return false;

    FILE* file = fopen(path, "rb");
    if (file == NULL) return false;

    fseek(file, 0, SEEK_END);
    const long file_size = ftell(file);
    const size_t size = file_size > 0 ? (size_t)file_size : 0;

    size_t want = size;
    *truncated = false;
    if (want > LOG_READ_MAX)
    {
        want = LOG_READ_MAX;
        *truncated = true;
        fseek(file, (long)(size - want), SEEK_SET);
    }
    else
    {
        fseek(file, 0, SEEK_SET);
    }

    char* buffer = malloc(want + 1);
    if (buffer == NULL)
    {
        fclose(file);
        LOG_ERROR("读取日志时分配内存失败");
        return false;
    }

    const size_t read_len = fread(buffer, 1, want, file);
    fclose(file);
    buffer[read_len] = '\0';

    // 截断时第一行通常是残行, 直接丢掉
    if (*truncated)
    {
        char* first_break = strchr(buffer, '\n');
        if (first_break != NULL) memmove(buffer, first_break + 1, strlen(first_break + 1) + 1);
    }

    *out = buffer;
    *out_len = strlen(buffer);
    return true;
}

/**
 * @brief 处理 GET API 请求
 * @return 是否已处理 (未处理时需要继续走静态文件逻辑)
 */
static bool handle_api_get(struct mg_connection* c, struct mg_http_message* hm)
{
    // 根目录转发到 index.html
    if (mg_match(hm->uri, mg_str("/"), NULL))
    {
        mg_http_reply(c, 302, "Location: /index.html\r\n", "");
        return true;
    }

    // 获取认证状态
    if (mg_match(hm->uri, mg_str("/api/status/auth"), NULL))
    {
        bool is_authed = false;
        const bool reachable = query_is_authed(&is_authed);

        cJSON* auth = cJSON_CreateObject();
        cJSON_AddBoolToObject(auth, "status", is_authed);
        // 认证进程不可达时额外标出来, 前端可据此提示 (不认这个字段的前端会忽略它)
        cJSON_AddBoolToObject(auth, "reachable", reachable);
        char* status_str = cJSON_Print(auth);
        mg_http_reply(c, 200, HEADER_JSON, "%s", status_str);
        free(status_str);
        cJSON_Delete(auth);
        return true;
    }

    // 获取联网状态
    if (mg_match(hm->uri, mg_str("/api/status/online"), NULL))
    {
        switch (check_network_status(true))
        {
        case STATUS_OK:
            mg_http_reply(c, 204, "", "");
            break;
        case STATUS_NEED_AUTH:
            mg_http_reply(c, 302, "", "");
            break;
        default:
            mg_http_reply(c, 503, "", "");
        }
        return true;
    }

    // 获取程序运行信息
    if (mg_match(hm->uri, mg_str("/api/status/sys"), NULL))
    {
        cJSON* info = cJSON_CreateObject();
        cJSON_AddStringToObject(info, "version", PROGRAM_FULL_VERSION);
        cJSON_AddNumberToObject(info, "uptime_ms", (double)(get_cur_tm_ms() - g_start_run_tm));
        cJSON_AddNumberToObject(info, "log_level", get_logger_level());
        cJSON_AddStringToObject(info, "log_dir", safe_str(get_logger_dir()));
        cJSON_AddStringToObject(info, "config_file", safe_str(get_config_file_path()));
        cJSON_AddBoolToObject(info, "program_enabled", g_prog_enabled);
        cJSON_AddBoolToObject(info, "restart_pending", g_need_restart);
        cJSON_AddNumberToObject(info, "account_count", g_prog_cnt);
        cJSON_AddNumberToObject(info, "thread_count", g_prog_cnt);
        char* info_str = cJSON_Print(info);
        mg_http_reply(c, 200, HEADER_JSON, "%s", info_str);
        free(info_str);
        cJSON_Delete(info);
        return true;
    }

    // 获取配置
    if (mg_match(hm->uri, mg_str("/api/getConfigs"), NULL))
    {
        cJSON* configs = cJSON_CreateObject();

        cJSON_AddBoolToObject(configs, "enabled", g_prog_enabled);
        cJSON_AddBoolToObject(configs, "web_external_acc", g_web_external_acc);
        cJSON_AddNumberToObject(configs, "log_lv", get_logger_level());
        /**
         * 这里给的是配置里的原文 (没写时是 "./"), 不是解析后的绝对路径:
         * 页面上的输入框要原样回显, 保存时也原样写回去, 不能把它改写成绝对路径
         * (看实际用的是哪个目录请走 /api/status/sys 的 log_dir)
         */
        cJSON_AddStringToObject(configs, "log_dir", get_logger_dir_cfg());
        cJSON_AddNumberToObject(configs, "conn_timeout", (double)g_conn_timeout);
        cJSON_AddNumberToObject(configs, "op_timeout", (double)g_op_timeout);
        cJSON_AddNumberToObject(configs, "web_port", g_web_port);

        cJSON* accounts = cJSON_CreateArray();
        cJSON* account = cJSON_CreateObject();

        cJSON_AddStringToObject(account, "username", g_prog_status[0].login_cfg.usr);
        cJSON_AddStringToObject(account, "password", g_prog_status[0].login_cfg.pwd);
        {
            const char* channel_name = "android";
            switch (g_prog_status[0].login_cfg.chn)
            {
            case 1:
                channel_name = "windows";
                break;
            case 2:
                channel_name = "linux";
                break;
            case 3:
                channel_name = "android";
                break;
            case 4:
                channel_name = "ios";
                break;
            case 5:
                channel_name = "macos";
                break;
            default:
                channel_name = "android";
                break;
            }
            cJSON_AddStringToObject(account, "channel", channel_name);
        }

        cJSON* time_windows = cJSON_CreateArray();
        for (uint8_t i = 0; i < g_prog_status[0].login_cfg.time_window_count; i++)
        {
            const time_window_t* win = &g_prog_status[0].login_cfg.time_windows[i];
            char start_str[TIME_WINDOW_STR_LEN];
            char end_str[TIME_WINDOW_STR_LEN];
            format_week_min(win->start_week_min, start_str);
            format_week_min(win->end_week_min, end_str);

            cJSON* window_obj = cJSON_CreateObject();
            cJSON_AddStringToObject(window_obj, "start", start_str);
            cJSON_AddStringToObject(window_obj, "end", end_str);
            cJSON_AddItemToArray(time_windows, window_obj);
        }
        cJSON_AddItemToObject(account, "time_windows", time_windows);

        cJSON_AddItemToArray(accounts, account);
        cJSON_AddItemToObject(configs, "accounts", accounts);

        char* config_str = cJSON_Print(configs);

        mg_http_reply(c, 200, HEADER_JSON, "%s", config_str);

        free(config_str);
        cJSON_Delete(configs);
        return true;
    }

    // 获取日志文件列表 / 日志内容
    if (mg_match(hm->uri, mg_str("/api/logs"), NULL))
    {
        char file_name[LOG_NAME_LEN];
        const int name_len = mg_http_get_var(&hm->query, "file", file_name, sizeof(file_name));

        // 带 file 参数: 返回文件内容
        if (name_len > 0)
        {
            if (is_safe_log_name(file_name) == false)
            {
                mg_http_reply(c, 400, HEADER_TEXT, "非法的日志文件名\n");
                return true;
            }

            char* content = NULL;
            size_t content_len = 0;
            bool truncated = false;
            if (read_log_file(file_name, &content, &content_len, &truncated) == false)
            {
                mg_http_reply(c, 404, HEADER_TEXT, "日志文件不存在或已被轮转\n");
                return true;
            }

            LOG_DEBUG("Web 读取日志文件 %s (%zu 字节%s)", file_name, content_len, truncated ? ", 已截断" : "");
            mg_http_reply(c, 200, truncated ? HEADER_TEXT_TRUNCATED : HEADER_TEXT, "%s", content);
            free(content);
            return true;
        }

        // 不带参数: 返回文件列表
        log_file_entry_t entries[LOG_FILE_MAX];
        const int count = list_log_files(entries, LOG_FILE_MAX);
        if (count > 1) qsort(entries, (size_t)count, sizeof(log_file_entry_t), cmp_log_files);

        cJSON* root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "dir", safe_str(get_logger_dir()));
        cJSON* files = cJSON_CreateArray();
        for (int i = 0; i < count; i++)
        {
            cJSON* file = cJSON_CreateObject();
            cJSON_AddStringToObject(file, "name", entries[i].name);
            cJSON_AddNumberToObject(file, "size", (double)entries[i].size);
            cJSON_AddNumberToObject(file, "mtime", (double)entries[i].mtime);
            cJSON_AddBoolToObject(file, "current", entries[i].current);
            cJSON_AddItemToArray(files, file);
        }
        cJSON_AddItemToObject(root, "files", files);

        char* files_str = cJSON_Print(root);
        mg_http_reply(c, 200, HEADER_JSON, "%s", files_str);
        free(files_str);
        cJSON_Delete(root);
        return true;
    }

    return false;
}

/**
 * @brief 处理 POST API 请求
 * @return 是否已处理
 */
static bool handle_api_post(struct mg_connection* c, struct mg_http_message* hm)
{
    // 从 Web 线程读取请求体
    char* data = NULL;
    if (hm->body.len > 0)
    {
        data = malloc(hm->body.len + 1);
        if (data == NULL)
        {
            mg_http_reply(c, 500, "", "");
            return true;
        }
        memcpy(data, hm->body.buf, hm->body.len);
        data[hm->body.len] = '\0';
    }

    // 仅保存
    if (mg_match(hm->uri, mg_str("/api/saveConfigs"), NULL))
    {
        if (data == NULL)
        {
            mg_http_reply(c, 400, "", "");
            return true;
        }
        if (save_cfg(data))
        {
            mg_http_reply(c, 204, "", "");
        }
        else
        {
            mg_http_reply(c, 500, "", "");
        }
        free(data);
        return true;
    }

    // 仅应用
    if (mg_match(hm->uri, mg_str("/api/applyConfigs"), NULL))
    {
        if (data == NULL)
        {
            mg_http_reply(c, 400, "", "");
            return true;
        }

        cJSON* operation_json = cJSON_Parse(data);
        if (operation_json == NULL)
        {
            mg_http_reply(c, 500, "", "");
            free(data);
            return true;
        }

        const cJSON* apply = cJSON_GetObjectItem(operation_json, "apply");
        if (apply)
        {
            if (cJSON_IsBool(apply))
            {
                if (g_prog_status == NULL)
                {
                    LOG_WARN("收到 Web 应用配置文件请求, 但程序尚未加载完成, 拒绝操作");
                    mg_http_reply(c, 403, "", "");
                }
                else
                {
                    bool is_authed = false;
                    const bool reachable = query_is_authed(&is_authed);

                    if (reachable == false)
                    {
                        LOG_WARN("收到 Web 应用配置文件请求, 但认证进程不可达");
                        mg_http_reply(c, 503, "", "");
                    }
                    else if (is_authed == false)
                    {
                        LOG_WARN("收到 Web 应用配置文件请求, 但没有线程在认证, 拒绝操作");
                        mg_http_reply(c, 403, "", "");
                    }
                    else if (request_apply_config() == false)
                    {
                        LOG_WARN("收到 Web 应用配置文件请求, 但下发到认证进程失败");
                        mg_http_reply(c, 503, "", "");
                    }
                    else
                    {
                        LOG_INFO("收到 Web 应用配置文件请求, 程序将重新加载配置文件并进行认证");
                        mg_http_reply(c, 204, "", "");
                    }
                }
            }
            else
            {
                mg_http_reply(c, 500, "", "");
            }
        }
        else
        {
            mg_http_reply(c, 500, "", "");
        }

        free(data);
        cJSON_Delete(operation_json);
        return true;
    }

    // 重新认证
    if (mg_match(hm->uri, mg_str("/api/restartAuth"), NULL))
    {
        if (g_prog_status == NULL || g_cfg_loaded == false)
        {
            LOG_WARN("收到 Web 重新认证请求, 但程序尚未加载配置, 拒绝操作");
            mg_http_reply(c, 503, "", "");
        }
        else
        {
            bool is_authed = false;
            const bool reachable = query_is_authed(&is_authed);

            if (reachable == false)
            {
                LOG_WARN("收到 Web 重新认证请求, 但认证进程不可达");
                mg_http_reply(c, 503, "", "");
            }
            else if (is_authed == false)
            {
                LOG_WARN("收到 Web 重新认证请求, 但没有线程在认证, 拒绝操作");
                mg_http_reply(c, 403, "", "");
            }
            else if (request_restart_auth() == false)
            {
                LOG_WARN("收到 Web 重新认证请求, 但下发到认证进程失败");
                mg_http_reply(c, 503, "", "");
            }
            else
            {
                LOG_INFO("收到 Web 重新认证请求, 认证线程将重新进行认证");
                mg_http_reply(c, 204, "", "");
            }
        }

        free(data);
        return true;
    }

    free(data);

    // 未知的 POST 路径
    mg_http_reply(c, 404, HEADER_TEXT, "Not found\n");
    return true;
}

/**
 * @brief 网页文件所在目录
 *
 * 必须按【程序所在目录】拼绝对路径, 不能写成相对路径 "portal":
 * 那样它会相对于进程的当前工作目录, 而配置文件是按程序目录找的, 两者不一致。
 * 把程序放进 /usr/local/bin 再从别处启动 (教程里就这么建议的) 时,
 * 网页文件会找不到, 打开页面只有 404。
 * @return 目录路径
 */
static const char* web_root_dir(void)
{
    static char dir[PATH_MAX] = "";

    if (dir[0] != '\0') return dir;

    char exec_dir[PATH_MAX];
    if (get_exec_dir(exec_dir) == false)
    {
        LOG_WARN("无法获取程序所在目录, 网页文件将按当前目录下的 portal 查找");
        return "portal";
    }

    snprintf(dir, sizeof(dir), "%s%cportal", safe_str(exec_dir), SEP);
    return dir;
}

static void fn(struct mg_connection *c, const int ev, void *ev_data)
{
    if (ev != MG_EV_HTTP_MSG) return;

    struct mg_http_message* hm = ev_data;

    // GET 请求
    if (mg_strcmp(hm->method, mg_str("GET")) == 0)
    {
        // 命中 API 时直接返回, 避免静态文件处理重复发送响应
        if (handle_api_get(c, hm)) return;

            struct mg_http_serve_opts opts = { .root_dir = web_root_dir() };
        mg_http_serve_dir(c, hm, &opts);
        return;
    }

    // POST 请求
    if (mg_strcmp(hm->method, mg_str("POST")) == 0)
    {
        handle_api_post(c, hm);
    }
}

static void logFn(const char ch, void *param)
{
    (void)param;
    static char buffer[512];
    static size_t pos = 0;
    if (ch == '\n' || pos >= sizeof(buffer) - 1)
    {
        if (pos > 0)
        {
            const char* web_log_level = strchr(buffer, ' ');
            if (!web_log_level)
            {
                LOG_WARN("未知的 Web 日志: %s", buffer);
                return;
            }
            const char* file_start = web_log_level + 3;
            const char* file_end = strchr(file_start, ':');
            if (!file_end)
            {
                LOG_WARN("未知的 Web 日志: %s", buffer);
                return;
            }
            const size_t file_length = file_end - file_start;
            char* file = malloc(file_length + 1);
            if (!file)
            {
                LOG_WARN("分配内存失败");
                return;
            }
            memcpy(file, file_start, file_length);
            file[file_length] = '\0';
            const char* file_line_start = file_end + 1;
            const char* file_line_end = strchr(file_line_start, ':');
            if (!file_line_end)
            {
                LOG_WARN("未知的 Web 日志: %s", buffer);
                return;
            }
            const size_t file_line_length = file_line_end - file_line_start;
            char* file_line_str = malloc(file_line_length + 1);
            if (!file_line_str)
            {
                LOG_WARN("分配内存失败");
                return;
            }
            memcpy(file_line_str, file_line_start, file_line_length);
            file_line_str[file_line_length] = '\0';
            const uint64_t file_line = str2uint64(file_line_str);
            const char* msg = file_line_end + 1;
            switch(web_log_level[1])
            {
            case '1':
                LOG_WEB_ERROR(file, file_line, "%s", msg);
                break;
            case '2':
                LOG_WEB_INFO(file, file_line, "%s", msg);
                break;
            default:
                LOG_WEB_VERBOSE(file, file_line, "%s", msg);
            }
            free(file);
            free(file_line_str);
        }
        pos = 0;
    }
    else if (ch != '\r')
    {
        buffer[pos++] = ch;
    }
}

/** @brief 监听地址缓冲区长度 (形如 "http://0.0.0.0:65535") */
#define WEB_LISTEN_LEN 64

static int web_server(void* arg)
{
    tl_thread_idx = (int8_t)(intptr_t)arg;
    tl_thread_name = "web"; // 日志里标成本线程, 不要把标签交给魔术数字去猜
    struct mg_mgr mgr;
    mg_log_level = MG_LL_VERBOSE;
    mg_log_set_fn(logFn, NULL);
    mg_mgr_init(&mgr);

    /**
     * 监听地址由配置文件决定: web_port 是端口, web_external_acc 决定只监听回环
     * 还是监听全部网卡。
     *
     * 默认只监听回环: /api/getConfigs 会返回明文账号密码, 而服务本身没有鉴权,
     * 监听 0.0.0.0 等于把这些暴露给整个局域网 —— 开启外部访问的时候要说一声
     */
    char listen_addr[WEB_LISTEN_LEN];
    snprintf(listen_addr, sizeof(listen_addr), "http://%s:%" PRIu16,
        g_web_external_acc ? "0.0.0.0" : "127.0.0.1", g_web_port);

    if (mg_http_listen(&mgr, listen_addr, fn, NULL) == NULL)
    {
        LOG_FATAL("Web 服务监听失败: %s (端口可能已被占用)", listen_addr);
        mg_mgr_free(&mgr);
        return 1;
    }

    if (g_web_external_acc)
    {
        LOG_WARN("Web 服务已允许外部访问 (%s), 而接口没有鉴权且会返回明文账号密码, 请确认这确实是你想要的", listen_addr);
    }

    g_is_webserver_running = 1;
    LOG_INFO("Web 服务器已启动, 访问地址: %s", listen_addr);
    while (g_is_webserver_running) mg_mgr_poll(&mgr, 1000);
    mg_mgr_free(&mgr);
    LOG_INFO("Web 服务器已停止");
    return 0;
}

/**
 * @brief 启动 Web 服务线程
 * @return 是否启动成功
 */
static bool start_web_thread()
{
    web_thread = sim_thread_create(web_server, (void*)(intptr_t)-2);

    uint8_t retry = 1;
    while (web_thread == NULL)
    {
        if (retry > 5)
        {
            LOG_FATAL("超过重试次数");
            return false;
        }
        LOG_ERROR("Web 服务器线程创建失败, 重试中, 重试次数: %" PRIu8 ", 最多 5 次", retry);
        web_thread = sim_thread_create(web_server, (void*)(intptr_t)-2);
        retry++;
    }
    return true;
}

bool start_web_server()
{
    s_local_state = true;
    return start_web_thread();
}

bool start_web_server_remote()
{
    control_set_port(g_control_port);
    s_local_state = false;

    LOG_INFO("Web 进程模式: 认证状态通过控制通道 127.0.0.1:%" PRIu16 " 获取", g_control_port);

    return start_web_thread();
}

void stop_web_server()
{
    g_is_webserver_running = 0;
    int result_code = 0;
    sim_thread_join(web_thread, &result_code);
    LOG_DEBUG("Web 服务器线程退出, 退出码: %d", result_code);
}
