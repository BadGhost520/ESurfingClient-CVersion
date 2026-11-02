#ifndef ESURFINGCLIENT_NETTRAFFIC_H
#define ESURFINGCLIENT_NETTRAFFIC_H

#include <stdbool.h>

/**
 * 实时网速采样 (Windows / Linux / macOS; OpenWrt 不编)
 *
 * 每秒取一次各网卡的累计收发字节数, 与上一帧做差得到 B/s, 供网页的
 * "实时网速"卡片使用; 采样在本进程线程里做, 不依赖外部脚本或子进程。
 */

/**
 * @brief 启动采样线程 (重复调用安全)
 *
 * 先立刻采一帧建立基线 (这一帧没有速率), 之后每秒一帧
 * @return 是否启动成功 (平台不支持或线程创建失败时为 false)
 */
bool net_traffic_start(void);

/**
 * @brief 停止采样线程并等它退出 (没启动过时是空操作)
 */
void net_traffic_stop(void);

/**
 * @brief 取一帧采样结果的 JSON
 *
 * 形如 {"timestamp":<毫秒>,"defaultId":"<网卡id>","adapters":[{"id","name","down","up"}],
 * "hotspotControls":<bool>}; 首帧或计数器重置时 down/up 为 null (界面显示"正在采样")
 * @return JSON 文本 (需要 free), 采样尚未建立时为 NULL
 */
char* net_traffic_json(void);

#endif
