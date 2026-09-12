#ifndef ESURFINGCLIENT_TIMECONTROL_H
#define ESURFINGCLIENT_TIMECONTROL_H

#include <stdbool.h>

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
 * @brief 停止时间控制定时线程并等待退出
 */
void time_control_stop(void);

#endif // ESURFINGCLIENT_TIMECONTROL_H
