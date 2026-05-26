#include "utils/PlatformUtils.h"
#include "webserver/WebServer.h"
#include "webserver/mongoose.h"
#include "utils/SimThread.h"
#include "utils/Logger.h"
#include "NetClient.h"
#include "States.h"

static const char* listenAddr = "http://0.0.0.0:8888";
static sim_thread_t* web_thread;

static void fn(struct mg_connection *c, const int ev, void *ev_data)
{
    if (ev == MG_EV_HTTP_MSG)
    {
        struct mg_http_message *hm = ev_data;
        struct mg_http_serve_opts opts = { .root_dir = "portal" };
        // GET 请求
        if (mg_strcmp(hm->method, mg_str("GET")) == 0)
        {
            // 根目录
            if (mg_match(hm->uri, mg_str("/"), NULL))
            {
                hm->uri = mg_str("/dashboard.html");
            }
            mg_http_serve_dir(c, hm, &opts);
            return;
        }
        // POST 请求
        if (mg_strcmp(hm->method, mg_str("POST")) == 0)
        {
            if (mg_match(hm->uri, mg_str("/api/test"), NULL))
            {

            }
        }
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

int web_server(void* arg)
{
    thread_idx = (int8_t)(intptr_t)arg;
    struct mg_mgr mgr;
    mg_log_level = MG_LL_VERBOSE;
    mg_log_set_fn(logFn, NULL);
    mg_mgr_init(&mgr);

    mg_http_listen(&mgr, listenAddr, fn, NULL);
    is_webserver_running = 1;
    LOG_INFO("Web 服务器已启动, 后台访问地址: http://127.0.0.1:8888/");
    while (is_webserver_running) mg_mgr_poll(&mgr, 1000);
    mg_mgr_free(&mgr);
    LOG_INFO("Web 服务器已停止");
    return 0;
}

bool start_web_server()
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

void stop_web_server()
{
    is_webserver_running = 0;
    int result_code = 0;
    sim_thread_join(web_thread, &result_code);
    LOG_DEBUG("Web 服务器线程退出, 退出码: %d", result_code);
}