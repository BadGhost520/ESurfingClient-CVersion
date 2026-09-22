#ifndef ESURFINGCLIENT_CONTROL_H
#define ESURFINGCLIENT_CONTROL_H

#include <stdbool.h>
#include <stdint.h>

#define CONTROL_DEFAULT_PORT 8890

#define CONTROL_MSG_MAX 1024

typedef struct
{
    uint8_t account;
    bool is_authed;
    bool is_running;
    bool is_time_disabled;
} control_status_t;

/* -------------------- 认证进程侧 -------------------- */

/**
 * @brief 启动控制服务
 * @param port 监听端口
 * @return 是否启动成功 (端口被占用时返回 false, 调用方可降级为不提供服务)
 */
bool control_server_start(uint16_t port);

/**
 * @brief 停止控制服务并等待线程退出
 */
void control_server_stop(void);

/* -------------------- Web 进程侧 -------------------- */

/**
 * @brief 设置要连接的控制端口
 * @param port 端口
 */
void control_set_port(uint16_t port);

/**
 * @brief 查询认证进程的运行时状态
 * @param out 状态快照
 * @return 是否查询成功 (认证进程没起来时为 false)
 */
bool control_query_status(control_status_t* out);

/**
 * @brief 请求认证进程重新认证
 * @return 是否下发成功
 */
bool control_restart_auth(void);

/**
 * @brief 请求认证进程重新加载配置并重新认证
 * @return 是否下发成功
 */
bool control_apply_config(void);

/**
 * @brief 请求认证进程优雅退出 (监管者停止子进程时用)
 *
 * 为什么需要它: Windows 上没有 SIGTERM, 监管者原本靠 GenerateConsoleCtrlEvent
 * 发 CTRL_BREAK。但那个 API 要求调用方与目标在同一个控制台, 而**服务由 SCM
 * 启动时根本没有控制台**, 于是这一路必然失败, 只能 TerminateProcess 强杀 ——
 * 认证子进程跑不到 clean(), 也就不会登出。
 *
 * 控制通道走的是回环 TCP, 不依赖控制台, 三种场景 (服务 / 控制台 / POSIX)
 * 行为一致, 因此作为首选路径; 发不出去时监管者再退回原来的办法。
 * @return 是否下发成功 (子进程没监听控制端口时为 false)
 */
bool control_request_shutdown(void);

#endif
