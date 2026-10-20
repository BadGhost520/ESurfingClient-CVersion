#ifndef ESURFINGCLIENT_NETTRAFFIC_H
#define ESURFINGCLIENT_NETTRAFFIC_H

#include <stdbool.h>

/**
 * 实时网速采样 (Windows; 其它平台暂未实现)
 *
 * 每秒向系统要一次各网卡的累计收发字节数, 与上一帧做差得到 B/s,
 * 网页上的"实时网速"卡片就是靠它。放在程序自己的线程里采,
 * 不再需要外部脚本、子进程, 也不用往 portal 目录写 traffic.json。
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
