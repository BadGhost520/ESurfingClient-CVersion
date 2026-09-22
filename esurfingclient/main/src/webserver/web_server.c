#include "webserver/WebServer.h"

#include "webserver/web_internal.h"

#include <mongoose/mongoose.h>

#include "control/Control.h"

#include "states/States.h"

#include "utils/sim/SimThread.h"

#include "utils/Logger.h"

static sim_thread_t* web_thread;

bool s_local_state = true;

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
