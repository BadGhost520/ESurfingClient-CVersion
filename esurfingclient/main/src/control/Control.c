#include "control/Control.h"

#include "utils/PlatformUtils.h"
#include "utils/Logger.h"
#include "utils/cJSON.h"
#include "utils/sim/SimThread.h"

#include "States.h"

#include <string.h>
#include <stdlib.h>

/**
 * 注意包含顺序: Windows 下必须先于 windows.h 引入 winsock2.h,
 * 否则会与 windows.h 里的 winsock1 冲突
 */
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET ctl_sock_t;
#define CTL_INVALID_SOCK INVALID_SOCKET
#define ctl_close closesocket
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
typedef int ctl_sock_t;
#define CTL_INVALID_SOCK (-1)
#define ctl_close close
#endif

/** @brief 控制线程在日志里的标识 */
#define TL_IDX_CONTROL (-3)

/** @brief 等待单条报文的超时 (毫秒) */
#define CTL_TIMEOUT_MS 3000

/** @brief 认证进程侧: 是否正在提供服务 */
static volatile bool s_server_running = false;

/** @brief 认证进程侧: 监听套接字 */
static ctl_sock_t s_listen_sock = CTL_INVALID_SOCK;

/** @brief 认证进程侧: 监听端口 */
static uint16_t s_server_port = CONTROL_DEFAULT_PORT;

/** @brief 认证进程侧: 服务线程 */
static sim_thread_t* s_server_thread = NULL;

/** @brief Web 进程侧: 要连接的控制端口 */
static uint16_t s_client_port = CONTROL_DEFAULT_PORT;

/* ------------------------------------------------------------------
 * 通用
 * ------------------------------------------------------------------ */

#ifdef _WIN32
static bool ctl_net_init()
{
    /**
     * WSAStartup 是带引用计数的, 每调一次都要有对应的 WSACleanup。
     * 每次请求都调它的话计数只增不减 (Web 进程每刷一次页面就调一次),
     * 属于记账式泄漏。初始化一次就够, 进程退出时由系统统一回收。
     */
    static bool s_wsa_ready = false;
    if (s_wsa_ready) return true;

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return false;

    s_wsa_ready = true;
    return true;
}
#else
static bool ctl_net_init()
{
    return true;
}
#endif

/**
 * @brief 等待套接字可读
 * @param sock 套接字
 * @param timeout_ms 超时毫秒数
 * @return 1 可读, 0 超时, -1 出错
 */
static int ctl_wait_readable(const ctl_sock_t sock, const long timeout_ms)
{
    fd_set read_set;
    FD_ZERO(&read_set);
    FD_SET(sock, &read_set);

    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    const int ready = select((int)sock + 1, &read_set, NULL, NULL, &tv);
    if (ready <= 0) return ready;

    return FD_ISSET(sock, &read_set) ? 1 : 0;
}

/**
 * @brief 读取一行报文
 * @param sock 套接字
 * @param buf 缓冲
 * @param buf_len 缓冲长度
 * @return 读到的长度, 0 表示对端关闭或超时
 */
static size_t ctl_read_line(const ctl_sock_t sock, char* buf, const size_t buf_len)
{
    size_t used = 0;

    while (used + 1 < buf_len)
    {
        if (ctl_wait_readable(sock, CTL_TIMEOUT_MS) != 1) break;

        const int n = recv(sock, buf + used, (int)(buf_len - 1 - used), 0);
        if (n <= 0) break;

        used += (size_t)n;
        if (memchr(buf, '\n', used) != NULL) break;
    }

    buf[used] = '\0';
    return used;
}

/* ------------------------------------------------------------------
 * 认证进程侧: 控制服务
 * ------------------------------------------------------------------ */

/**
 * @brief 组装状态应答
 * @param reply 输出缓冲
 * @param reply_len 缓冲长度
 */
static void ctl_reply_status(char* reply, const size_t reply_len)
{
    cJSON* root = cJSON_CreateObject();
    cJSON* data = cJSON_CreateObject();

    /**
     * 这里读的是认证线程在写的运行时状态。
     * 沿用项目现有的跨线程裸 bool 风格 (与 TimeControl 一致),
     * 都是单字长标量, 且只有认证线程与这里的读取方
     */
    uint8_t account = 0;
    bool is_authed = false;
    bool is_running = false;
    bool is_time_disabled = false;

    if (g_prog_status != NULL && g_prog_cnt > 0)
    {
        account = g_prog_status[0].login_cfg.idx;
        is_authed = g_prog_status[0].runtime_status.is_authed;
        is_running = g_prog_status[0].runtime_status.is_running;
        is_time_disabled = g_prog_status[0].runtime_status.is_time_disabled;
    }

    cJSON_AddNumberToObject(data, "account", account);
    cJSON_AddBoolToObject(data, "is_authed", is_authed);
    cJSON_AddBoolToObject(data, "is_running", is_running);
    cJSON_AddBoolToObject(data, "is_time_disabled", is_time_disabled);

    cJSON_AddBoolToObject(root, "ok", true);
    cJSON_AddItemToObject(root, "data", data);

    char* text = cJSON_PrintUnformatted(root);
    snprintf(reply, reply_len, "%s\n", safe_str(text));
    free(text);
    cJSON_Delete(root);
}

/**
 * @brief 组装简单应答
 * @param reply 输出缓冲
 * @param reply_len 缓冲长度
 * @param ok 是否成功
 * @param error 失败原因 (成功时可为 NULL)
 */
static void ctl_reply_result(char* reply, const size_t reply_len, const bool ok, const char* error)
{
    cJSON* root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "ok", ok);
    if (ok == false) cJSON_AddStringToObject(root, "error", safe_str(error));

    char* text = cJSON_PrintUnformatted(root);
    snprintf(reply, reply_len, "%s\n", safe_str(text));
    free(text);
    cJSON_Delete(root);
}

/**
 * @brief 校验请求令牌
 *
 * 控制通道只监听回环, 但本机其它进程同样连得上;
 * 没配令牌时不做校验 (手工单独起进程的场景)
 * @param req 请求对象
 * @return 令牌是否正确
 */
static bool ctl_check_token(const cJSON* req)
{
    if (g_control_token[0] == '\0') return true;

    const cJSON* item = cJSON_GetObjectItem(req, "token");
    if (item == NULL || cJSON_IsString(item) == false) return false;

    return strcmp(item->valuestring, g_control_token) == 0;
}

/**
 * @brief 处理一条请求并生成应答
 * @param request 请求报文
 * @param reply 输出缓冲
 * @param reply_len 缓冲长度
 */
static void ctl_dispatch(const char* request, char* reply, const size_t reply_len)
{
    cJSON* req = cJSON_Parse(request);
    if (req == NULL)
    {
        ctl_reply_result(reply, reply_len, false, "bad json");
        return;
    }

    if (ctl_check_token(req) == false)
    {
        LOG_WARN("控制通道收到令牌不匹配的请求, 已拒绝");
        ctl_reply_result(reply, reply_len, false, "bad token");
        cJSON_Delete(req);
        return;
    }

    const cJSON* cmd_item = cJSON_GetObjectItem(req, "cmd");
    const char* cmd = (cmd_item != NULL && cJSON_IsString(cmd_item)) ? cmd_item->valuestring : "";

    if (strcmp(cmd, "status") == 0)
    {
        ctl_reply_status(reply, reply_len);
    }
    else if (strcmp(cmd, "restart_auth") == 0)
    {
        if (g_prog_status == NULL || g_prog_cnt <= 0)
        {
            ctl_reply_result(reply, reply_len, false, "config not loaded");
        }
        else
        {
            LOG_INFO("控制通道收到重新认证请求");
            g_prog_status[0].runtime_status.is_need_reauth = true;
            ctl_reply_result(reply, reply_len, true, NULL);
        }
    }
    else if (strcmp(cmd, "apply_config") == 0)
    {
        if (g_prog_status == NULL || g_prog_cnt <= 0)
        {
            ctl_reply_result(reply, reply_len, false, "config not loaded");
        }
        else
        {
            LOG_INFO("控制通道收到应用新配置请求");
            g_cfg_loaded = false;
            g_prog_status[0].runtime_status.is_need_reauth = true;
            ctl_reply_result(reply, reply_len, true, NULL);
        }
    }
    else if (strcmp(cmd, "shutdown") == 0)
    {
        /**
         * 只置标志, 不在这里调 shut(): 本函数就跑在控制服务线程上,
         * 而 shut() 会 control_server_stop() 去 join 这条线程 —— 自己等自己。
         * 主循环看到 g_stop_requested 后退出, 由 dialer_app 跑完 clean() (含登出)
         */
        LOG_INFO("控制通道收到退出请求, 本进程将正常关闭");
        g_stop_requested = 1;
        ctl_reply_result(reply, reply_len, true, NULL);
    }
    else
    {
        ctl_reply_result(reply, reply_len, false, "unknown cmd");
    }

    cJSON_Delete(req);
}

/**
 * @brief 处理一个客户端连接
 * @param client 客户端套接字
 */
static void ctl_handle_client(const ctl_sock_t client)
{
    char request[CONTROL_MSG_MAX];
    char reply[CONTROL_MSG_MAX];

    if (ctl_read_line(client, request, sizeof(request)) == 0) return;

    ctl_dispatch(request, reply, sizeof(reply));

    if (send(client, reply, (int)strlen(reply), 0) <= 0)
    {
        LOG_DEBUG("控制通道应答发送失败");
    }
}

/**
 * @brief 控制服务线程主循环
 */
static int ctl_server_app(void* arg)
{
    (void)arg;
    tl_thread_idx = TL_IDX_CONTROL;
    tl_thread_name = "control"; // 日志里标成本线程, 不要把标签交给魔术数字去猜

    LOG_INFO("控制通道服务线程已启动");

    while (s_server_running)
    {
        if (ctl_wait_readable(s_listen_sock, 1000) != 1) continue;

        const ctl_sock_t client = accept(s_listen_sock, NULL, NULL);
        if (client == CTL_INVALID_SOCK) continue;

        ctl_handle_client(client);
        ctl_close(client);
    }

    LOG_INFO("控制通道已停止");
    return 0;
}

bool control_server_start(const uint16_t port)
{
    if (ctl_net_init() == false)
    {
        LOG_ERROR("网络库初始化失败, 控制通道无法启动");
        return false;
    }

    s_listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (s_listen_sock == CTL_INVALID_SOCK)
    {
        LOG_ERROR("创建控制通道套接字失败");
        return false;
    }

    // 只监听回环: 控制通道不对外开放
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (bind(s_listen_sock, (struct sockaddr*)&addr, sizeof(addr)) != 0)
    {
        LOG_ERROR("控制通道绑定 127.0.0.1:%" PRIu16 " 失败 (端口可能已被占用)", port);
        ctl_close(s_listen_sock);
        s_listen_sock = CTL_INVALID_SOCK;
        return false;
    }

    if (listen(s_listen_sock, 4) != 0)
    {
        LOG_ERROR("控制通道监听失败");
        ctl_close(s_listen_sock);
        s_listen_sock = CTL_INVALID_SOCK;
        return false;
    }

    s_server_running = true;
    s_server_port = port;
    s_server_thread = sim_thread_create(ctl_server_app, NULL);
    if (s_server_thread == NULL)
    {
        LOG_ERROR("控制通道线程创建失败");
        s_server_running = false;
        ctl_close(s_listen_sock);
        s_listen_sock = CTL_INVALID_SOCK;
        return false;
    }

    LOG_INFO("控制通道已启动, 监听 127.0.0.1:%" PRIu16 "%s", s_server_port,
        g_control_token[0] != '\0' ? " (需要令牌)" : " (未设令牌, 本机任意进程都可下发动作)");
    return true;
}

void control_server_stop(void)
{
    if (s_server_thread == NULL) return;

    s_server_running = false;

    int result_code = 0;
    sim_thread_join(s_server_thread, &result_code);
    s_server_thread = NULL;

    if (s_listen_sock != CTL_INVALID_SOCK)
    {
        ctl_close(s_listen_sock);
        s_listen_sock = CTL_INVALID_SOCK;
    }
}

/* ------------------------------------------------------------------
 * Web 进程侧: 控制客户端
 * ------------------------------------------------------------------ */

void control_set_port(const uint16_t port)
{
    s_client_port = port;
}

/**
 * @brief 连接控制通道并发出一条请求
 * @param cmd 命令名
 * @param reply 应答缓冲
 * @param reply_len 缓冲长度
 * @return 是否成功拿到应答
 */
static bool ctl_client_request(const char* cmd, char* reply, const size_t reply_len)
{
    char request[CONTROL_MSG_MAX];

    if (g_control_token[0] != '\0')
    {
        snprintf(request, sizeof(request), "{\"cmd\":\"%s\",\"token\":\"%s\"}\n", cmd, g_control_token);
    }
    else
    {
        snprintf(request, sizeof(request), "{\"cmd\":\"%s\"}\n", cmd);
    }

    if (ctl_net_init() == false) return false;

    const ctl_sock_t sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == CTL_INVALID_SOCK) return false;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(s_client_port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    /**
     * 连回环地址不需要连接超时: 对端没在监听时会立刻返回 ECONNREFUSED,
     * 因此阻塞 connect 不会卡住
     */
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) != 0)
    {
        ctl_close(sock);
        return false;
    }

    if (send(sock, request, (int)strlen(request), 0) <= 0)
    {
        ctl_close(sock);
        return false;
    }

    const size_t got = ctl_read_line(sock, reply, reply_len);
    ctl_close(sock);

    return got > 0;
}

/**
 * @brief 判断应答是否成功
 * @param reply 应答报文
 * @return 是否成功
 */
static bool ctl_reply_ok(const char* reply)
{
    cJSON* root = cJSON_Parse(reply);
    if (root == NULL) return false;

    const cJSON* ok_item = cJSON_GetObjectItem(root, "ok");
    const bool ok = (ok_item != NULL && cJSON_IsTrue(ok_item));

    cJSON_Delete(root);
    return ok;
}

bool control_query_status(control_status_t* out)
{
    if (out == NULL) return false;

    char reply[CONTROL_MSG_MAX];
    if (ctl_client_request("status", reply, sizeof(reply)) == false) return false;

    cJSON* root = cJSON_Parse(reply);
    if (root == NULL) return false;

    const cJSON* ok_item = cJSON_GetObjectItem(root, "ok");
    const cJSON* data = cJSON_GetObjectItem(root, "data");

    bool parsed = false;
    if (ok_item != NULL && cJSON_IsTrue(ok_item) && data != NULL)
    {
        const cJSON* account = cJSON_GetObjectItem(data, "account");
        const cJSON* is_authed = cJSON_GetObjectItem(data, "is_authed");
        const cJSON* is_running = cJSON_GetObjectItem(data, "is_running");
        const cJSON* is_time_disabled = cJSON_GetObjectItem(data, "is_time_disabled");

        out->account = (account != NULL && cJSON_IsNumber(account)) ? (uint8_t)account->valueint : 0;
        out->is_authed = (is_authed != NULL && cJSON_IsTrue(is_authed));
        out->is_running = (is_running != NULL && cJSON_IsTrue(is_running));
        out->is_time_disabled = (is_time_disabled != NULL && cJSON_IsTrue(is_time_disabled));
        parsed = true;
    }

    cJSON_Delete(root);
    return parsed;
}

bool control_restart_auth(void)
{
    char reply[CONTROL_MSG_MAX];
    if (ctl_client_request("restart_auth", reply, sizeof(reply)) == false) return false;
    return ctl_reply_ok(reply);
}

bool control_apply_config(void)
{
    char reply[CONTROL_MSG_MAX];
    if (ctl_client_request("apply_config", reply, sizeof(reply)) == false) return false;
    return ctl_reply_ok(reply);
}

bool control_request_shutdown(void)
{
    char reply[CONTROL_MSG_MAX];
    if (ctl_client_request("shutdown", reply, sizeof(reply)) == false) return false;
    return ctl_reply_ok(reply);
}
