#ifndef ESURFINGCLIENT_CONTROL_H
#define ESURFINGCLIENT_CONTROL_H

#include <stdbool.h>
#include <stdint.h>

/**
 * 控制通道 - 认证进程与 Web 进程之间的本机通信
 *
 * 进程拆分之后, Web 进程既看不到认证状态, 也不能直接改认证线程的运行时状态,
 * 因此开一条只监听本机回环的控制通道:
 *
 *   请求 (一行 JSON)          响应 (一行 JSON)
 *   {"cmd":"status"}         {"ok":true,"data":{...}}
 *   {"cmd":"restart_auth"}   {"ok":true}
 *   {"cmd":"apply_config"}   {"ok":true}
 *
 * 刻意不放配置读写: 两个进程读同一份配置文件, Web 进程保存配置也直接写文件,
 * 通道只负责"运行时状态"与"动作", 这样协议面最小、也最不容易出错。
 *
 * 只监听回环地址, 不做鉴权: 能连上本机回环端口的进程与本进程权限相同。
 */

/** @brief 默认控制端口 */
#define CONTROL_DEFAULT_PORT 8890

/** @brief 单条报文的最大长度 */
#define CONTROL_MSG_MAX 1024

/** @brief 认证进程的运行时状态快照 */
typedef struct
{
    /** @brief 负责的配置序号 */
    uint8_t account;
    /** @brief 是否已认证 */
    bool is_authed;
    /** @brief 认证线程是否在运行 */
    bool is_running;
    /** @brief 是否不在允许时段 */
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

#endif //ESURFINGCLIENT_CONTROL_H
