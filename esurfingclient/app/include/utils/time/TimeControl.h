#ifndef ESURFINGCLIENT_TIMECONTROL_H
#define ESURFINGCLIENT_TIMECONTROL_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief 启动时间控制定时线程
 * @return 是否启动成功 (没有任何时间控制账号时也返回 true)
 */
bool time_control_init(void);

/**
 * @brief 根据当前本地时间重新同步各账号的时间控制禁用状态
 * @note 冷启动和定时线程每次醒来后都会调用
 */
void time_control_sync(void);

/**
 * @brief 计算下一次需要重新校正时间控制状态前可以睡多久
 *
 * 认证进程没有独立的定时线程, 由认证线程按这个间隔自己校正,
 * 因此这里把"睡多久"单独暴露出来
 * @return 毫秒数 (没有启用时间控制的账号时返回一个短暂的轮询间隔)
 */
uint64_t time_control_wait_ms(void);

/**
 * @brief 停止时间控制定时线程并等待退出
 */
void time_control_stop(void);

#endif
