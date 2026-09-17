#ifndef ESURFINGCLIENT_WATCHDOG_H
#define ESURFINGCLIENT_WATCHDOG_H

#include <stdbool.h>
#include <stdint.h>

/**
 * 看门狗 - 让进程自己发现"我还活着但已经不干活了"
 *
 * 为什么需要它:
 *   外部监管者 (procd / systemd / SCM) 只能看到"进程退出了没有".
 *   进程卡死 (死锁、无超时的阻塞调用、绕不出来的死循环) 时它照样活着,
 *   监管者认为一切正常, 永远不会重启 —— 账号就那么一直挂着.
 *   OpenWrt 上尤其明显: 那边连监管进程都没有, 只有 procd 在看进程在不在.
 *
 * 怎么判定"卡住":
 *   绝不能做成"多久没打日志就重启" —— 本程序有大量合法的长时间静默, 比如
 *   "不在允许时段, 等待 3600000 毫秒后重新检查". 那样会误杀正常等待.
 *
 *   所以由主循环【主动声明】:
 *       watchdog_pet(30000);   // 我还活着, 接下来最多 30 秒不会再来打卡
 *   看门狗只在超过声明的时间上限 (再加一点余量) 还没等到下一次打卡时才判定卡死.
 *
 * 判定卡死后:
 *   尽力往日志写一行说明, 然后 _exit(1) —— 退出后外部监管者按 respawn 策略
 *   重新拉起. 不能调 shut(): 它会 join 线程, 可能正好卡在同一个死锁上.
 */

/** @brief 看门狗判定卡死后写进日志的内容前缀 (测试与文档都靠它识别) */
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

#endif //ESURFINGCLIENT_WATCHDOG_H
