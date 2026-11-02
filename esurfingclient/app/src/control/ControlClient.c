#include "control/Control.h"

#include "core/States.h"


#include <cJSON/cJSON.h>

#include <stdio.h>

#include "ControlInternal.h"

#ifndef _WIN32
#include <fcntl.h>
#endif

/* ------------------------------------------------------------------
 * Web 进程侧: 控制客户端
 * ------------------------------------------------------------------ */

/**
 * @brief 非阻塞地连上控制通道 (最多等 CTL_CONNECT_TIMEOUT_MS)
 *
 * 对端不在时不能靠阻塞 connect 兜底: 有些 Windows 机器连未被监听的本地端口要
 * 2 秒才回 ECONNREFUSED, 而 Web 线程是单线程的 —— 那 2 秒会把 /api/traffic 与
 * /api/hotspot/status 一起拖到超时 (页面上就是"读取超时"与"采集数据已过期")。
 * @return 是否连上
 */
static bool ctl_client_connect(const ctl_sock_t sock, const struct sockaddr_in* addr)
{
#ifdef _WIN32
    u_long nonblocking = 1;
    if (ioctlsocket(sock, FIONBIO, &nonblocking) != 0) return false;
#else
    const int flags = fcntl(sock, F_GETFL, 0);
    if (flags < 0 || fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0) return false;
#endif

    bool connected = false;

    if (connect(sock, (const struct sockaddr*)addr, sizeof(*addr)) == 0)
    {
        connected = true;
    }
    else if (ctl_wait_writable(sock, CTL_CONNECT_TIMEOUT_MS) == 1)
    {
        int error = 0;
#ifdef _WIN32
        int error_len = (int)sizeof(error);
        if (getsockopt(sock, SOL_SOCKET, SO_ERROR, (char*)&error, &error_len) == 0 && error == 0)
#else
        socklen_t error_len = sizeof(error);
        if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &error_len) == 0 && error == 0)
#endif
        {
            connected = true;
        }
    }

#ifdef _WIN32
    nonblocking = 0;
    ioctlsocket(sock, FIONBIO, &nonblocking);
#else
    fcntl(sock, F_SETFL, flags);
#endif

    return connected;
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
    addr.sin_port = htons(g_client_port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (ctl_client_connect(sock, &addr) == false)
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
        const cJSON* boot_id = cJSON_GetObjectItem(data, "bootId");

        out->account = (account != NULL && cJSON_IsNumber(account)) ? (uint8_t)account->valueint : 0;
        out->is_authed = (is_authed != NULL && cJSON_IsTrue(is_authed));
        out->is_running = (is_running != NULL && cJSON_IsTrue(is_running));
        out->is_time_disabled = (is_time_disabled != NULL && cJSON_IsTrue(is_time_disabled));
        snprintf(out->boot_id, sizeof(out->boot_id), "%s",
                 (boot_id != NULL && cJSON_IsString(boot_id)) ? boot_id->valuestring : "");
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
