#ifndef ESURFINGCLIENT_CONTROLINTERNAL_H
#define ESURFINGCLIENT_CONTROLINTERNAL_H

#include "core/States.h"

#ifdef _WIN32
#include <winsock2.h>
typedef SOCKET ctl_sock_t;
#define CTL_INVALID_SOCK INVALID_SOCKET
#define ctl_close closesocket
#define ctl_shutdown_write(sock) shutdown((sock), SD_SEND)
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
typedef int ctl_sock_t;
#define CTL_INVALID_SOCK (-1)
#define ctl_close close
#define ctl_shutdown_write(sock) shutdown((sock), SHUT_WR)
#endif

extern uint16_t g_client_port;

/**
 * 连接控制通道的等待上限
 *
 * 回环上正常的连接是亚毫秒级; 这个值只是兜住"端口不可达"的慢路径 ——
 * 有些 Windows 机器连未被监听的本地端口要 2 秒才回 ECONNREFUSED,
 * 而 Web 线程是单线程的, 那 2 秒会把所有接口一起拖住。
 */
#define CTL_CONNECT_TIMEOUT_MS 300

bool ctl_net_init(void);

size_t ctl_read_line(const ctl_sock_t sock, char* buf, const size_t buf_len);

/**
 * @brief 等待套接字可写 (非阻塞 connect 用)
 * @return 1 可写, 0 超时, -1 出错
 */
int ctl_wait_writable(ctl_sock_t sock, long timeout_ms);

/**
 * @brief 该端口现在能不能绑定
 *
 * 刻意不设 SO_REUSEADDR: 要的是"能不能真的绑上", 处于 TIME_WAIT 的端口
 * 也应算作不可用, 否则子进程真去绑的时候还是会失败。
 */
bool ctl_port_available(uint16_t port);

/**
 * @brief 等对端把连接关掉 (最多 timeout_ms)
 *
 * 服务端应答之后用它: 只有让客户端先 close, TIME_WAIT 才会落在客户端的临时端口上,
 * 而不是落在控制端口本身上 (见 ctl_handle_client 的说明)
 */
void ctl_wait_close(ctl_sock_t sock, long timeout_ms);

#endif
