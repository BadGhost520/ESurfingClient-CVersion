#include "utils/PlatformUtils.h"
#include "webserver/WebServer.h"
#include "webserver/mongoose.h"
#include "utils/incbin.h"
#include "utils/Logger.h"
#include "utils/cJSON.h"
#include "NetClient.h"
#include "States.h"

// HTML
INCTXT(web_index, WEB_ROOT_PATH "/index.html");
INCTXT(web_dashboard, WEB_ROOT_PATH "/assets/web/dashboard.html");
INCTXT(web_logs, WEB_ROOT_PATH "/assets/web/logs.html");
INCTXT(web_settings, WEB_ROOT_PATH "/assets/web/settings.html");
INCTXT(web_about, WEB_ROOT_PATH "/assets/web/about.html");
// SVG
INCTXT(vector_more, WEB_ROOT_PATH "/assets/vector/more.svg");
INCTXT(vector_loading, WEB_ROOT_PATH "/assets/vector/loading.svg");
INCTXT(vector_dashboard, WEB_ROOT_PATH "/assets/vector/dashboard.svg");
INCTXT(vector_logs, WEB_ROOT_PATH "/assets/vector/logs.svg");
INCTXT(vector_settings, WEB_ROOT_PATH "/assets/vector/settings.svg");
INCTXT(vector_about, WEB_ROOT_PATH "/assets/vector/about.svg");
// JavaScript
INCTXT(js_basic, WEB_ROOT_PATH "/assets/js/basic.js");
INCTXT(js_dashboard, WEB_ROOT_PATH "/assets/js/dashboard.js");
INCTXT(js_logs, WEB_ROOT_PATH "/assets/js/logs.js");
INCTXT(js_settings, WEB_ROOT_PATH "/assets/js/settings.js");
INCTXT(js_about, WEB_ROOT_PATH "/assets/js/about.js");
INCBIN(import_js_axios, WEB_ROOT_PATH "/assets/import-js/axios.min.js");
// CSS
INCTXT(css_basic, WEB_ROOT_PATH "/assets/css/basic.css");
INCTXT(css_sidebar, WEB_ROOT_PATH "/assets/css/sidebar.css");
INCTXT(css_dashboard, WEB_ROOT_PATH "/assets/css/dashboard.css");
INCTXT(css_logs, WEB_ROOT_PATH "/assets/css/logs.css");
INCTXT(css_settings, WEB_ROOT_PATH "/assets/css/settings.css");
INCTXT(css_about, WEB_ROOT_PATH "/assets/css/about.css");
// LICENSE
INCTXT(license, WEB_ROOT_PATH "/assets/license/LICENSE");

int is_webserver_running = 0;
static const char* listenAddr = "http://0.0.0.0:8888";
static NetworkStatus network_status = {0};

static void fn(struct mg_connection *c, const int ev, void *ev_data)
{
    if (ev == MG_EV_HTTP_MSG)
    {
        struct mg_http_message *hm = ev_data;
        // 网页内容请求
        if (mg_match(hm->uri, mg_str("/index.html"), NULL))
        {
            mg_http_reply(c,
                200,
            "Content-Type: text/html; charset=utf-8\r\n",
            gweb_indexData);
        }
        if (mg_match(hm->uri, mg_str("/dashboard"), NULL) || mg_match(hm->uri, mg_str("/"), NULL))
        {
            mg_http_reply(c,
                200,
            "Content-Type: text/html; charset=utf-8\r\n",
            gweb_dashboardData);
        }
        if (mg_match(hm->uri, mg_str("/logs"), NULL))
        {
            mg_http_reply(c,
                200,
            "Content-Type: text/html; charset=utf-8\r\n",
            gweb_logsData);
        }
        if (mg_match(hm->uri, mg_str("/settings"), NULL))
        {
            mg_http_reply(c,
                200,
            "Content-Type: text/html; charset=utf-8\r\n",
            gweb_settingsData);
        }
        if (mg_match(hm->uri, mg_str("/about"), NULL))
        {
            mg_http_reply(c,
                200,
            "Content-Type: text/html; charset=utf-8\r\n",
            gweb_aboutData);
        }
        if (mg_match(hm->uri, mg_str("/vector/more.svg"), NULL))
        {
            mg_http_reply(c,
                200,
            "Content-Type: image/svg+xml\r\n",
            gvector_moreData);
        }
        if (mg_match(hm->uri, mg_str("/vector/loading.svg"), NULL))
        {
            mg_http_reply(c,
                200,
            "Content-Type: image/svg+xml\r\n",
            gvector_loadingData);
        }
        if (mg_match(hm->uri, mg_str("/vector/dashboard.svg"), NULL))
        {
            mg_http_reply(c,
                200,
            "Content-Type: image/svg+xml\r\n",
            gvector_dashboardData);
        }
        if (mg_match(hm->uri, mg_str("/vector/logs.svg"), NULL))
        {
            mg_http_reply(c,
                200,
            "Content-Type: image/svg+xml\r\n",
            gvector_logsData);
        }
        if (mg_match(hm->uri, mg_str("/vector/settings.svg"), NULL))
        {
            mg_http_reply(c,
                200,
            "Content-Type: image/svg+xml\r\n",
            gvector_settingsData);
        }
        if (mg_match(hm->uri, mg_str("/vector/about.svg"), NULL))
        {
            mg_http_reply(c,
                200,
            "Content-Type: image/svg+xml\r\n",
            gvector_aboutData);
        }
        if (mg_match(hm->uri, mg_str("/js/basic.js"), NULL))
        {
            mg_http_reply(c,
                200,
            "Content-Type: application/javascript\r\n",
            gjs_basicData);
        }
        if (mg_match(hm->uri, mg_str("/js/dashboard.js"), NULL))
        {
            mg_http_reply(c,
                200,
            "Content-Type: application/javascript\r\n",
            gjs_dashboardData);
        }
        if (mg_match(hm->uri, mg_str("/js/logs.js"), NULL))
        {
            mg_http_reply(c,
                200,
            "Content-Type: application/javascript\r\n",
            gjs_logsData);
        }
        if (mg_match(hm->uri, mg_str("/js/settings.js"), NULL))
        {
            mg_http_reply(c,
                200,
            "Content-Type: application/javascript\r\n",
            gjs_settingsData);
        }
        if (mg_match(hm->uri, mg_str("/js/about.js"), NULL))
        {
            mg_http_reply(c,
                200,
            "Content-Type: application/javascript\r\n",
            gjs_aboutData);
        }
        if (mg_match(hm->uri, mg_str("/import-js/axios.min.js"), NULL))
        {
            mg_http_reply(c,
                200,
            "Content-Type: application/javascript\r\n",
            "%s",
            gimport_js_axiosData);
        }
        if (mg_match(hm->uri, mg_str("/css/basic.css"), NULL))
        {
            mg_http_reply(c,
                200,
            "Content-Type: text/css\r\n",
            gcss_basicData);
        }
        if (mg_match(hm->uri, mg_str("/css/sidebar.css"), NULL))
        {
            mg_http_reply(c,
                200,
            "Content-Type: text/css\r\n",
            gcss_sidebarData);
        }
        if (mg_match(hm->uri, mg_str("/css/dashboard.css"), NULL))
        {
            mg_http_reply(c,
                200,
            "Content-Type: text/css\r\n",
            gcss_dashboardData);
        }
        if (mg_match(hm->uri, mg_str("/css/logs.css"), NULL))
        {
            mg_http_reply(c,
                200,
            "Content-Type: text/css\r\n",
            gcss_logsData);
        }
        if (mg_match(hm->uri, mg_str("/css/settings.css"), NULL))
        {
            mg_http_reply(c,
                200,
            "Content-Type: text/css\r\n",
            gcss_settingsData);
        }
        if (mg_match(hm->uri, mg_str("/css/about.css"), NULL))
        {
            mg_http_reply(c,
                200,
            "Content-Type: text/css\r\n",
            gcss_aboutData);
        }
        if (mg_match(hm->uri, mg_str("/license/LICENSE"), NULL))
        {
            mg_http_reply(c,
                200,
            "Content-Type: text/plain; charset=utf-8\r\n",
            glicenseData);
        }
        // GET 请求
        if (mg_strcmp(hm->method, mg_str("GET")) == 0)
        {
            // 获取联网状态信息
            // if (mg_match(hm->uri, mg_str("/api/getNetworkStatus"), NULL))
            // {
            //     network_status = checkNetworkStatus();
            //     switch (network_status)
            //     {
            //     case REQUEST_SUCCESS:
            //         connection_status.is_connected = true;
            //         if (connection_status.connect_time == 0) connection_status.connect_time = currentTimeMillis();
            //         break;
            //     default:
            //         connection_status.is_connected = false;
            //         connection_status.connect_time = 0;
            //     }
            //     cJSON* status = cJSON_CreateObject();
            //     cJSON_AddBoolToObject(status, "isConnected", connection_status.is_connected);
            //     cJSON_AddStringToObject(status, "connectTime", longLongToString(connection_status.connect_time));
            //     char* response = cJSON_Print(status);
            //     cJSON_Delete(status);
            //     mg_http_reply(c,
            //         200,
            //         "Content-Type: application/json;charset=utf-8\r\n",
            //         "%s",
            //         response);
            //     cJSON_free(response);
            // }
            // 获取软件状态信息
            // if (mg_match(hm->uri, mg_str("/api/getSoftwareStatus"), NULL))
            // {
            //     cJSON* software_status = cJSON_CreateObject();
            //     cJSON* threads = cJSON_CreateArray();
            //     for (int i = 0; i < MAX_DIALER_COUNT; i++)
            //     {
            //         cJSON* thread = cJSON_CreateObject();
            //         cJSON_AddBoolToObject(thread, "isRunning", thread_status[i].dialer_context.runtime_status.is_running);
            //         cJSON_AddItemToArray(threads, thread);
            //     }
            //     cJSON_AddItemToObject(software_status, "threads", threads);
            //     cJSON_AddStringToObject(software_status, "runningTime", longLongToString(g_running_time));
            //     cJSON_AddStringToObject(software_status, "version", VERSION);
            //     char* response = cJSON_Print(software_status);
            //     cJSON_Delete(software_status);
            //     mg_http_reply(c,
            //         200,
            //         "Content-Type: application/json;charset=utf-8\r\n",
            //         "%s",
            //         response);
            //     cJSON_free(response);
            // }
            // 获取线程信息
            // if (mg_match(hm->uri, mg_str("/api/getThreadStatus"), NULL))
            // {
            //     cJSON* threads_status = cJSON_CreateObject();
            //     cJSON* threads = cJSON_CreateArray();
            //     for (int i = 0; i < MAX_DIALER_COUNT; i++)
            //     {
            //         cJSON* thread = cJSON_CreateObject();
            //         cJSON_AddBoolToObject(thread, "isAuth", thread_status[i].dialer_context.runtime_status.is_authed);
            //         cJSON_AddStringToObject(thread, "authTime", longLongToString(thread_status[i].dialer_context.auth_time));
            //         cJSON_AddStringToObject(thread, "userIp", thread_status[i].dialer_context.auth_config.client_ip);
            //         cJSON_AddBoolToObject(thread, "isRunning", thread_status[i].dialer_context.runtime_status.is_running);
            //         cJSON_AddItemToArray(threads, thread);
            //     }
            //     cJSON_AddItemToObject(threads_status, "threads", threads);
            //     char* response = cJSON_Print(threads_status);
            //     cJSON_Delete(threads_status);
            //     mg_http_reply(c,
            //         200,
            //         "Content-Type: application/json;charset=utf-8\r\n",
            //         "%s",
            //         response);
            //     cJSON_free(response);
            // }
            // 获取适配器状态
            if (mg_match(hm->uri, mg_str("/api/getAdapterInfo"), NULL))
            {
                char* response = getAdaptersJSON();
                mg_http_reply(c,
                    200,
                    "Content-Type: application/json;charset=utf-8\r\n",
                    "%s",
                    response);
                cJSON_free(response);
            }
            // 更新内存日志内容
            if (mg_match(hm->uri, mg_str("/api/updateLogs"), NULL))
            {
                const LogContent response = getLog(true);
                if (response.is_new)
                {
                    mg_http_reply(c,
                        200,
                        "Content-Type: text/plain;charset=utf-8\r\n",
                        "%s",
                    response.data);
                }
                else
                {
                    mg_http_reply(c,
                        204,
                        "Content-Type: text/plain;charset=utf-8\r\n",
                        "");
                }
                if (response.data) free(response.data);
            }
            // 获取内存日志内容
            if (mg_match(hm->uri, mg_str("/api/getLogs"), NULL))
            {
                const LogContent response = getLog(false);
                mg_http_reply(c,
                    200,
                    "Content-Type: text/plain;charset=utf-8\r\n",
                    "%s",
                response.data);
                if (response.data) free(response.data);
            }
            // 获取当前设置
            // if (mg_match(hm->uri, mg_str("/api/getSettings"), NULL))
            // {
            //     cJSON* threads_settings = cJSON_CreateObject();
            //     cJSON* threads = cJSON_CreateArray();
            //     for (int i = 0; i < MAX_DIALER_COUNT; i++)
            //     {
            //         cJSON* thread = cJSON_CreateObject();
            //         cJSON_AddStringToObject(thread, "username", prog_status[prog_index].login_config.usr);
            //         cJSON_AddStringToObject(thread, "password", prog_status[prog_index].login_config.pwd);
            //         cJSON_AddStringToObject(thread, "channel", prog_status[prog_index].login_config.chn);
            //         cJSON_AddItemToArray(threads, thread);
            //     }
            //     cJSON_AddItemToObject(threads_settings, "threads", threads);
            //     cJSON_AddBoolToObject(threads_settings, "debug", getLoggerLevel());
            //     char* response = cJSON_Print(threads_settings);
            //     cJSON_Delete(threads_settings);
            //     mg_http_reply(c,
            //         200,
            //         "Content-Type: application/json;charset=utf-8\r\n",
            //         "%s",
            //         response);
            //     cJSON_free(response);
            // }
        }
        // POST 请求
        if (mg_strcmp(hm->method, mg_str("POST")) == 0)
        {
            // 启动或关闭指定线程的认证程序
            // if (mg_match(hm->uri, mg_str("/api/manageThread"), NULL))
            // {
            //     const struct mg_str body = hm->body;
            //     cJSON* manageThread = cJSON_Parse(body.buf);
            //     if (manageThread)
            //     {
            //         const cJSON* index = cJSON_GetObjectItem(manageThread, "index");
            //         if (index && cJSON_IsNumber(index))
            //         {
            //             const int tmp_index = index->valueint;
            //             if (thread_status[tmp_index].dialer_context.runtime_status.is_running)
            //             {
            //                 LOG_INFO("等待线程 %d 认证程序关闭", tmp_index + 1);
            //                 restartThread(tmp_index);
            //                 mg_http_reply(c,
            //                     204,
            //                     "",
            //                     "");
            //             }
            //             else
            //             {
            //                 LOG_INFO("正在启动认证程序, 序号: %d", tmp_index + 1);
            //                 thread_status[tmp_index].dialer_context.runtime_status.is_running = true;
            //                 if (thread_status[tmp_index].dialer_context.runtime_status.is_running)
            //                 {
            //                     LOG_INFO("认证程序启动成功, 序号: %d", tmp_index + 1);
            //                     mg_http_reply(c,
            //                         204,
            //                         "",
            //                         "");
            //                 }
            //                 else
            //                 {
            //                     LOG_ERROR("认证程序启动失败, 序号: %d", tmp_index + 1);
            //                     mg_http_reply(c,
            //                         400,
            //                         "Content-Type: text/plain;charset=utf-8\r\n",
            //                         "启动认证程序失败");
            //                 }
            //             }
            //         }
            //         cJSON_Delete(manageThread);
            //     }
            //     else
            //     {
            //         mg_http_reply(c,
            //             400,
            //             "Content-Type: text/plain;charset=utf-8\r\n",
            //             "启动认证线程失败, 原因: 解析 JSON 失败");
            //     }
            // }
            // 更新线程设置
            // if (mg_match(hm->uri, mg_str("/api/updateThreadSettings"), NULL))
            // {
            //     const struct mg_str body = hm->body;
            //     cJSON* updateThread = cJSON_Parse(body.buf);
            //     if (updateThread)
            //     {
            //         const cJSON* username = cJSON_GetObjectItem(updateThread, "username");
            //         const cJSON* password = cJSON_GetObjectItem(updateThread, "password");
            //         const cJSON* channel = cJSON_GetObjectItem(updateThread, "channel");
            //         const cJSON* index = cJSON_GetObjectItem(updateThread, "index");
            //         if (index && cJSON_IsNumber(index))
            //         {
            //             if (username && cJSON_IsString(username)) snprintf(thread_status[index->valueint].dialer_context.adapter_config.usr, USR_LENGTH, "%s", username->valuestring);
            //             if (password && cJSON_IsString(password)) snprintf(thread_status[index->valueint].dialer_context.adapter_config.pwd, PWD_LENGTH, "%s", password->valuestring);
            //             if (channel && cJSON_IsString(channel)) snprintf(thread_status[index->valueint].dialer_context.adapter_config.chn, CHN_LENGTH, "%s", channel->valuestring);
            //             thread_status[index->valueint].dialer_context.runtime_status.is_settings_changed = true;
            //         }
            //         cJSON_Delete(updateThread);
            //         mg_http_reply(c,
            //             204,
            //             "",
            //             "");
            //     }
            //     else
            //     {
            //         mg_http_reply(c,
            //             400,
            //             "Content-Type: text/plain;charset=utf-8\r\n",
            //             "设置更新失败, 原因: 解析 JSON 失败");
            //     }
            // }
        }
    }
}

static void logFn(const char ch, void *param)
{
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
            const long long file_line = stringToUint64(file_line_str);
            const char* msg = file_line_end + 1;
            switch(web_log_level[1])
            {
            case '1':
                LOG_WEB_ERROR(file, file_line, "%s", msg);
                break;
            case '2':
                LOG_WEB_INFO(file, file_line, "%s", msg);
                break;
            case '3':
            case '4':
                LOG_WEB_VERBOSE(file, file_line, "%s", msg);
                break;
            default:
                LOG_WARN("未知等级的 Web 日志: %s", msg);
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

void startWebServer()
{
    struct mg_mgr mgr;
    mg_log_level = MG_LL_VERBOSE;
    mg_log_set_fn(logFn, NULL);
    mg_mgr_init(&mgr);
    mg_http_listen(&mgr, listenAddr, fn, NULL);
    is_webserver_running = 1;
    LOG_INFO("Web 服务器已启动，后台访问地址: http://127.0.0.1:8888/");
    while (is_webserver_running) mg_mgr_poll(&mgr, 1000);
    mg_mgr_free(&mgr);
    LOG_INFO("Web 服务器已停止");
}

void stopWebServer()
{
    is_webserver_running = 0;
}