#ifndef ESURFINGCLIENT_SUPERVISOR_H
#define ESURFINGCLIENT_SUPERVISOR_H

/**
 * @brief 让程序以监管进程启动的参数
 *
 * 安装系统服务时用它作为启动参数, 这样服务起来就是"监管者 + 认证 + Web"的多进程拓扑
 */
#define SUPERVISOR_ARG "--role supervisor"

/**
 * @brief 监管进程主流程 (仅桌面)
 *
 * 把认证与 Web 拆成独立进程之后, 需要有人把它们拉起来并看住:
 *
 * - 按配置里的可用账号拉起 N 个认证进程, 外加 1 个 Web 进程
 * - 子进程退出后按退避策略重新拉起 (存活够久就认为健康, 退避重置)
 * - 有序关闭: 先停 Web 进程 (不再接受新的控制请求), 再停认证进程
 *   (它们退出前会走一遍登出流程, 需要留时间)
 *
 * OpenWrt 上不使用本进程: 那里由 procd 每个账号起一个实例。
 *
 * @return 进程退出码
 */
int work_supervisor(void);

#endif //ESURFINGCLIENT_SUPERVISOR_H
