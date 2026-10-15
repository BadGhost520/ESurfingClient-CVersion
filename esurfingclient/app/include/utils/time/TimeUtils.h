#ifndef ESURFINGCLIENT_TIMEUTILS_H
#define ESURFINGCLIENT_TIMEUTILS_H

#include "core/States.h"

#include <stdbool.h>
#include <stdint.h>

/* 时间: 墙钟 / 单调钟 / 睡眠 / 格式化 */

/**
 * @brief 获取当前时间的毫秒时间戳 (墙钟, 会被 NTP / 改时钟影响, 只用于显示)
 * @return 64位时间戳
 */
uint64_t get_cur_tm_ms();

/**
 * @brief 获取单调递增的毫秒时间戳 (从系统启动算起, 不受改时钟影响)
 *
 * 凡是"测间隔"的地方都必须用它, 不能用 get_cur_tm_ms():
 * OpenWrt 没有 RTC, 开机后 NTP 一同步就会把墙钟整体前跳 (几十秒到几十年都可能),
 * 拿墙钟做减法的超时判断会当场误判。
 * @return 毫秒时间戳, 失败时返回 0
 */
uint64_t get_steady_tm_ms();

/**
 * @brief 睡眠
 * @param ms 毫秒
 * @param can_stop 能否被打断
 */
void sleep_ms(uint64_t ms, bool can_stop);

/**
 * @brief 获取当前时间
 * @param buf 时间戳缓冲区
 * @param fmt 格式
 */
void get_fmt_time(char* buf, TimeFormat fmt);

#endif
