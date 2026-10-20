#include "WebInternal.h"

#include "config/Config.h"
#include "control/Control.h"
#include "core/States.h"
#include "dialer/ErrorCode.h"
#include "net/NetClient.h"
#include "net/NetTraffic.h"
#include "utils/log/Logger.h"
#include "utils/platform/PlatformUtils.h"
#include "utils/text/TextUtils.h"
#include "utils/time/TimeUtils.h"
#include "webserver/HotspotControl.h"

#include <cJSON/cJSON.h>
#include <mongoose/mongoose.h>

#define LOG_FILE_MAX 64
#define LOG_READ_MAX (256 * 1024)

/**
 * @brief 把一个 cJSON 对象按 JSON 回给浏览器, 并负责释放它
 *
 * "Print → 回包 → free → Delete" 这套在每个接口里都写了一遍, 统一到这里,
 * 顺手也把 Print 失败 (内存不足) 的情况处理成 500
 */
static void reply_json(struct mg_connection* c, cJSON* root)
{
    if (root == NULL)
    {
        mg_http_reply(c, 500, "", "");
        return;
    }

    char* text = cJSON_Print(root);
    if (text == NULL)
    {
        mg_http_reply(c, 500, "", "");
    }
    else
    {
        mg_http_reply(c, 200, HEADER_JSON, "%s", text);
        free(text);
    }
    cJSON_Delete(root);
}

/**
 * @brief 查询认证状态
 * @param out 是否已认证
 * @return 是否查询成功 (Web 进程连不上认证进程时为 false)
 */
bool query_is_authed(bool* out)
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
 * @brief 读取日志文件内容
 * @param name 日志文件名
 * @param out 输出内容 (需要 free)
 * @param out_len 输出内容长度
 * @param truncated 是否被截断
 * @return 是否读取成功
 */
static bool read_log_file(const char* name, char** out, size_t* out_len, bool* truncated)
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
bool handle_api_get(struct mg_connection* c, struct mg_http_message* hm)
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

        /**
         * 查不到认证状态时, 说清楚是哪一种:
         *   config      配置还没就绪 (程序停在"等配置", 还没开始认证)
         *   no-account  配置读起来了, 但没有可用账号
         *   disabled    程序被 enabled=false 关掉了
         *   not-running 服务模式下连不上认证子进程
         * 前端据此换个说法, 不要一律显示成"认证进程未运行"
         */
        const char* reason = "";
        if (g_cfg_loaded == false)
        {
            // 配置还没读起来 (或读起来不可用): 程序停在"等配置", 认证根本没开始
            reason = "config";
        }
        else if (g_prog_enabled == false)
        {
            reason = "disabled";
        }
        else if (reachable == false)
        {
            reason = s_local_state ? "no-account" : "not-running";
        }

        cJSON* auth = cJSON_CreateObject();
        cJSON_AddBoolToObject(auth, "status", is_authed);
        // 认证进程不可达时额外标出来, 前端可据此提示 (不认这个字段的前端会忽略它)
        cJSON_AddBoolToObject(auth, "reachable", reachable);
        cJSON_AddStringToObject(auth, "reason", reason);

        // 上一次认证失败的服务端错误码与翻译, 让网页能直接显示"为什么失败"
        const char* error_code = "";
        const char* sub_error = "";
        const char* error_message = "";
        const char* error_kind = "";
        double error_at = 0;
        if (g_prog_status != NULL && g_prog_cnt > 0)
        {
            const runtime_status_t* status = &g_prog_status[0].runtime_status;
            error_code = status->last_error_code;
            sub_error = status->last_sub_error;
            error_message = status->last_error_msg;
            if (status->last_error_kind != AUTH_ERROR_NONE)
            {
                error_kind = auth_error_kind_name((auth_error_kind_t)status->last_error_kind);
            }
            error_at = (double)status->last_error_at;
        }
        cJSON_AddStringToObject(auth, "errorCode", error_code);
        cJSON_AddStringToObject(auth, "subError", sub_error);
        cJSON_AddStringToObject(auth, "errorMessage", error_message);
        cJSON_AddStringToObject(auth, "errorKind", error_kind);
        cJSON_AddNumberToObject(auth, "errorAt", error_at);
        reply_json(c, auth);
        return true;
    }

    // 实时网速: 由本程序自己的采样线程提供 (不再读 portal/assets/traffic.json)
    if (mg_match(hm->uri, mg_str("/api/traffic"), NULL))
    {
        char* traffic = net_traffic_json();
        if (traffic == NULL)
        {
            // 平台暂不支持, 或者采样还没建立起第一帧
            mg_http_reply(c, 503, "", "");
        }
        else
        {
            mg_http_reply(c, 200, HEADER_JSON, "%s", traffic);
            free(traffic);
        }
        return true;
    }

    // 热点状态: 读后台缓存的快照, 不在这里起 PowerShell, 免得把 Web 线程卡住
    if (mg_match(hm->uri, mg_str("/api/hotspot/status"), NULL))
    {
        char* hotspot = hotspot_status_json();
        if (hotspot == NULL)
        {
            mg_http_reply(c, 501, "", "");
        }
        else
        {
            mg_http_reply(c, 200, HEADER_JSON, "%s", hotspot);
            free(hotspot);
        }
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
        reply_json(c, info);
        return true;
    }

    // 获取配置
    if (mg_match(hm->uri, mg_str("/api/getConfigs"), NULL))
    {
        /**
         * 配置还没加载起来时 (停在"等配置"状态, 或者网页刚保存、重新加载还没跑完)
         * 直接返回配置文件里的内容: 内存里那份是空的或者旧的, 显示成空账号的话,
         * 用户一保存就把自己写过的账号密码覆盖掉了
         *
         * 文件内容要能解析才敢返回 —— 配置写坏时网页至少要能打开, 那时按内存里的
         * 默认值显示, 用户在网页上重新填一份保存就是
         */
        if (g_cfg_loaded == false)
        {
            char* cfg_text = read_cfg_file();
            if (cfg_text != NULL)
            {
                cJSON* probe = cJSON_Parse(cfg_text);
                if (probe != NULL)
                {
                    cJSON_Delete(probe);
                    mg_http_reply(c, 200, HEADER_JSON, "%s", cfg_text);
                    free(cfg_text);
                    return true;
                }
                free(cfg_text);
            }
        }

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
        cJSON_AddStringToObject(account, "channel", channel_name(g_prog_status[0].login_cfg.chn));

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

        reply_json(c, configs);
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

        reply_json(c, root);
        return true;
    }

    return false;
}
