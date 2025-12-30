#include <stdio.h>

#include "../headFiles/utils/PlatformUtils.h"
#include "../headFiles/utils/CheckAdapters.h"
#include "../headFiles/webserver/WebServer.h"
#include "../headFiles/webserver/mongoose.h"
#include "../headFiles/DialerClient.h"
#include "../headFiles/utils/incbin.h"
#include "../headFiles/utils/Logger.h"
#include "../headFiles/utils/cJSON.h"
#include "../headFiles/NetClient.h"
#include "../headFiles/States.h"

// HTML
INCTXT(web_index, WEB_ROOT_PATH "/index.html");
INCTXT(web_dashboard, WEB_ROOT_PATH "/assets/web/dashboard.html");
INCTXT(web_logs, WEB_ROOT_PATH "/assets/web/logs.html");
INCTXT(web_settings, WEB_ROOT_PATH "/assets/web/settings.html");
INCTXT(web_about, WEB_ROOT_PATH "/assets/web/about.html");
// SVG
INCTXT(vector_more, WEB_ROOT_PATH "/assets/vector/more.svg");
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
static ConnectionStatus connection_status = {0};
static const char* listenAddr = "http://0.0.0.0:8888";

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
            if (mg_match(hm->uri, mg_str("/api/getNetworkStatus"), NULL))
            {
                cJSON* status = cJSON_CreateObject();
                cJSON_AddBoolToObject(status, "isConnected", connection_status.is_connected);
                cJSON_AddStringToObject(status, "connectTime", longLongToString(connection_status.connect_time));
                char* response = cJSON_Print(status);
                cJSON_Delete(status);
                mg_http_reply(c,
                    200,
                    "Content-Type: application/json;charset=utf-8\r\n",
                    "%s",
                    response);
                cJSON_free(response);
            }
            // 获取软件状态信息
            if (mg_match(hm->uri, mg_str("/api/getSoftwareStatus"), NULL))
            {
                cJSON* software_status = cJSON_CreateObject();
                cJSON* threads = cJSON_CreateArray();
                for (int i = 0; i < MAX_DIALER_COUNT; i++)
                {
                    cJSON* thread = cJSON_CreateObject();
                    cJSON_AddBoolToObject(thread, "threadIsRunning", thread_status[i].thread_is_running);
                    cJSON_AddItemToArray(threads, thread);
                }
                cJSON_AddItemToObject(software_status, "threads", threads);
                cJSON_AddStringToObject(software_status, "runningTime", longLongToString(g_running_time));
                cJSON_AddStringToObject(software_status, "version", VERSION);
                char* response = cJSON_Print(software_status);
                cJSON_Delete(software_status);
                mg_http_reply(c,
                    200,
                    "Content-Type: application/json;charset=utf-8\r\n",
                    "%s",
                    response);
                cJSON_free(response);
            }
            // 获取线程信息
            if (mg_match(hm->uri, mg_str("/api/getThreadStatus"), NULL))
            {
                cJSON* threads_status = cJSON_CreateObject();
                cJSON* threads = cJSON_CreateArray();
                for (int i = 0; i < MAX_DIALER_COUNT; i++)
                {
                    cJSON* thread = cJSON_CreateObject();
                    cJSON_AddBoolToObject(thread, "isAuth", thread_status[i].dialer_context.runtime_status.is_authed);
                    cJSON_AddStringToObject(thread, "authTime", longLongToString(thread_status[i].dialer_context.auth_time));
                    cJSON_AddStringToObject(thread, "userIp", thread_status[i].dialer_context.auth_config.user_ip);
                    cJSON_AddBoolToObject(thread, "isRunning", thread_status[i].dialer_context.runtime_status.is_running);
                    cJSON_AddItemToArray(threads, thread);
                }
                cJSON_AddItemToObject(threads_status, "threads", threads);
                char* response = cJSON_Print(threads_status);
                cJSON_Delete(threads_status);
                mg_http_reply(c,
                    200,
                    "Content-Type: application/json;charset=utf-8\r\n",
                    "%s",
                    response);
                cJSON_free(response);
            }
            // 获取适配器状态
            if (mg_match(hm->uri, mg_str("/api/getAdapterInfo"), NULL))
            {
                char* response = getAdapterJSON();
                mg_http_reply(c,
                    200,
                    "Content-Type: application/json;charset=utf-8\r\n",
                    "%s",
                    response);
                cJSON_free(response);
            }
            // 获取内存日志内容
            if (mg_match(hm->uri, mg_str("/api/getLogs"), NULL))
            {
                const LogContent response = getLog();
                mg_http_reply(c,
                    200,
                    "Content-Type: text/plain;charset=utf-8\r\n",
                    "%s",
                    response.data);
                free(response.data);
            }
            // 获取当前设置
            if (mg_match(hm->uri, mg_str("/api/getSettings"), NULL))
            {
                cJSON* threads_settings = cJSON_CreateObject();
                cJSON* threads = cJSON_CreateArray();
                for (int i = 0; i < MAX_DIALER_COUNT; i++)
                {
                    cJSON* thread = cJSON_CreateObject();
                    cJSON_AddStringToObject(thread, "username", thread_status[i].dialer_context.options.usr);
                    cJSON_AddStringToObject(thread, "password", thread_status[i].dialer_context.options.pwd);
                    cJSON_AddStringToObject(thread, "channel", thread_status[i].dialer_context.options.chn);
                    cJSON_AddItemToArray(threads, thread);
                }
                cJSON_AddItemToObject(threads_settings, "threads", threads);
                cJSON_AddBoolToObject(threads_settings, "debug", getLoggerSettings());
                char* response = cJSON_Print(threads_settings);
                cJSON_Delete(threads_settings);
                mg_http_reply(c,
                    200,
                    "Content-Type: application/json;charset=utf-8\r\n",
                    "%s",
                    response);
                cJSON_free(response);
            }
        }
        // POST 请求
        if (mg_strcmp(hm->method, mg_str("POST")) == 0)
        {
            // 启动或关闭指定线程
            if (mg_match(hm->uri, mg_str("/api/manageThread"), NULL))
            {
                const struct mg_str body = hm->body;
                cJSON* manageThread = cJSON_Parse(body.buf);
                if (manageThread)
                {
                    const cJSON* index = cJSON_GetObjectItem(manageThread, "index");
                    if (index && cJSON_IsNumber(index))
                    {
                        const int tmp_index = index->valueint;
                        if (thread_status[tmp_index].thread_is_running)
                        {
                            LOG_INFO("等待子线程关闭");
                            thread_status[tmp_index].need_stop = true;
                            sleepMilliseconds(1000);
                            mg_http_reply(c,
                                204,
                                "",
                                "");
                        }
                        else
                        {
                            createThread(dialerApp, (void*)(intptr_t)tmp_index);
                            if (thread_status[tmp_index].thread_status == 0)
                            {
                                mg_http_reply(c,
                                    204,
                                    "",
                                    "");
                            }
                            else
                            {
                                LOG_ERROR("认证线程启动失败，序号 %d", tmp_index);
                                mg_http_reply(c,
                                    400,
                                    "Content-Type: text/plain;charset=utf-8\r\n",
                                    "启动认证线程失败, 状态码: %d",
                                    thread_status[tmp_index].thread_status);
                            }
                        }
                    }
                    cJSON_Delete(manageThread);
                }
                else
                {
                    mg_http_reply(c,
                        400,
                        "Content-Type: text/plain;charset=utf-8\r\n",
                        "启动认证线程失败, 原因: 解析 JSON 失败");
                }
            }
            // 更新线程设置
            if (mg_match(hm->uri, mg_str("/api/updateThreadSettings"), NULL))
            {
                const struct mg_str body = hm->body;
                cJSON* updateThread = cJSON_Parse(body.buf);
                if (updateThread)
                {
                    const cJSON* username = cJSON_GetObjectItem(updateThread, "username");
                    const cJSON* password = cJSON_GetObjectItem(updateThread, "password");
                    const cJSON* channel = cJSON_GetObjectItem(updateThread, "channel");
                    const cJSON* index = cJSON_GetObjectItem(updateThread, "index");
                    if (index && cJSON_IsNumber(index))
                    {
                        if (username && cJSON_IsString(username)) thread_status[index->valueint].dialer_context.options.usr = strdup(username->valuestring);
                        if (password && cJSON_IsString(password)) thread_status[index->valueint].dialer_context.options.pwd = strdup(password->valuestring);
                        if (channel && cJSON_IsString(channel)) thread_status[index->valueint].dialer_context.options.chn = strdup(channel->valuestring);
                        thread_status[index->valueint].dialer_context.runtime_status.is_settings_changed = true;
                    }
                    cJSON_Delete(updateThread);
                    mg_http_reply(c,
                        204,
                        "",
                        "");
                }
                else
                {
                    mg_http_reply(c,
                        400,
                        "Content-Type: text/plain;charset=utf-8\r\n",
                        "设置更新失败, 原因: 解析 JSON 失败");
                }
            }
        }
    }
}

void startWebServer()
{
    struct mg_mgr mgr;
    mg_mgr_init(&mgr);
    mg_http_listen(&mgr, listenAddr, fn, NULL);

    LOG_INFO("Web 服务器已启动，后台访问地址: http://127.0.0.1:8888/");

    is_webserver_running = 1;
    while (is_webserver_running)
    {
        switch (simGet("http://www.baidu.com"))
        {
        case REQUEST_SUCCESS:
            connection_status.is_connected = true;
            if (connection_status.connect_time == 0) connection_status.connect_time = currentTimeMillis();
            break;
        default:
            connection_status.is_connected = false;
            connection_status.connect_time = 0;
        }
        mg_mgr_poll(&mgr, 1000);
    }

    mg_mgr_free(&mgr);
    LOG_INFO("Web 服务器已停止");
}

void stopWebServer()
{
    is_webserver_running = 0;
}