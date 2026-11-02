#ifndef ESURFINGCLIENT_WATCHDOG_H
#define ESURFINGCLIENT_WATCHDOG_H

#include <stdbool.h>
#include <stdint.h>

#define WATCHDOG_KILL_MARK "看门狗判定本进程卡死"

/**
 * @brief 启动看门狗线程
 *
 * 只在需要它的角色里调用 (目前是认证进程). 重复调用是安全的.
 * @return 是否启动成功
 */
bool watchdog_start(void);

/**
 * @brief 停止看门狗 (关闭流程开始前调用, 免得把正常退出误判成卡死)
 */
void watchdog_stop(void);

/**
 * @brief 报告"我还活着", 并声明接下来最多多久不会再来报告
 *
 * 没启动看门狗时是空操作, 所以在共用代码里调用不会有副作用.
 * @param budget_ms 下一次打卡之前允许经过的最大毫秒数
 */
void watchdog_pet(uint32_t budget_ms);

/**
 * @brief 按配置的网络超时打一次卡 (发起网络请求前调用)
 *
 * 所有网络请求都走 NetClient 的 get() / post(), 那两个函数在真正发起请求前
 * 会调用本函数 —— 这样每一次可能长时间阻塞的网络调用都被逐个覆盖,
 * 不必去猜"一轮循环最多会做几次请求" (那种猜法一旦猜小就会误杀)。
 */
void watchdog_pet_network(void);

#endif
