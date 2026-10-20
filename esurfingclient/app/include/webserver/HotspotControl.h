#ifndef ESURFINGCLIENT_HOTSPOTCONTROL_H
#define ESURFINGCLIENT_HOTSPOTCONTROL_H

#include <stdbool.h>

/**
 * 手机热点开关 (Windows)
 *
 * 开关本身只有 WinRT 接口 (NetworkOperatorTetheringManager), C11 没有投影,
 * 所以每次操作都让随包的 HotspotControl.ps1 跑一次拿结果 —— 不再有常驻的
 * 桥接进程, 页面直接请求本程序的 /api/hotspot 下几个接口, 同源、不用令牌。
 *
 * 状态查询由后台线程定期跑一次并缓存, Web 请求只读缓存, 不会把 Web 线程卡住;
 * 开关动作也是异步的: 接口立刻返回"已受理", 页面靠轮询状态看到结果。
 */

/**
 * @brief 这个平台/这个包里能不能控制热点
 *
 * 只有 Windows 且随包带了 HotspotControl.ps1 才算能; 其它平台返回 false,
 * 网页上就不显示热点卡片 (net_traffic_json 里的 hotspotControls 字段)
 * @return 是否支持
 */
bool hotspot_control_available(void);

/**
 * @brief 启动后台状态刷新线程 (不支持时是空操作, 重复调用安全)
 */
void hotspot_control_start(void);

/**
 * @brief 停止后台线程 (没启动过时是空操作)
 */
void hotspot_control_stop(void);

/**
 * @brief 取热点状态 JSON (只读缓存, 不阻塞)
 *
 * 字段与页面约定的一致: adapters[{id,name,state,capability,connected}]、defaultId、
 * busy、message。缓存还没建立时返回"正在读取热点状态"的占位 JSON
 * @return JSON 文本 (需要 free); 平台不支持时返回 NULL
 */
char* hotspot_status_json(void);

/**
 * @brief 下发一次开关 (异步)
 * @param action "start" 或 "stop"
 * @param adapter_id 目标网卡 id (Windows 是网卡 GUID, Linux 是接口名)
 * @param out_json 返回给页面的消息 JSON (需要 free)
 * @return HTTP 状态码: 202 已受理 / 400 参数不对 / 409 正在切换 / 501 平台不支持
 */
int hotspot_request(const char* action, const char* adapter_id, char** out_json);

#endif
