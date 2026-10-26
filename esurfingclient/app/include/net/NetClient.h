#ifndef ESURFINGCLIENT_NETCLIENT_H
#define ESURFINGCLIENT_NETCLIENT_H

#include "Definition.h"

#include <cJSON/cJSON.h>

/**
 * @brief 带默认头的 POST
 * @param url 地址
 * @param data 数据
 * @return 响应数据
 */
curl_resp_t post(const char* url, const char* data);

/**
 * @brief 带默认头的 GET
 * @param url 地址
 * @return 响应数据
 */
curl_resp_t get(const char* url);

/**
 * @brief 释放一次响应里的响应体, 并把指针置空
 *
 * 用完一次响应就调一次: 直接 free(body_data) 忘了置空的话, 同一个 resp
 * 被复用时会重复释放
 * @param resp 响应 (可为 NULL)
 */
void curl_resp_free(curl_resp_t* resp);

/**
 * @brief 检测网络状态
 *
 * 并发探测一组 generate_204 地址 (单次短超时), 都不通再去试认证服务器。
 * @param connect_only 仅作兼容保留, 当前实现统一发完整 GET 再看状态码
 * @return 网络状态
 */
network_status_t check_network_status(bool connect_only);

/**
 * @brief 填内置的默认探测目标 (hicloud / miui 两组)
 *
 * 配置文件里没有 probe_hosts 时用这套; 有配置就由 probe_targets_parse 覆盖。
 */
void probe_targets_defaults(void);

/**
 * @brief 从配置解析探测目标
 *
 * 期望形如 `"probe_hosts": [{ "host": "...", "ips": ["1.2.3.4", "..."] }]`。
 * 解析失败 (不是数组 / 没有合法条目) 返回 false, 调用方保留默认值。
 * @param arr probe_hosts 数组
 * @return 是否成功覆盖
 */
bool probe_targets_parse(const cJSON* arr);

/**
 * @brief 把当前生效的探测目标导出成 probe_hosts 数组
 *
 * 用于把补全后的结果写回配置文件, 让用户始终能看到实际生效的目标。
 * @return 新建的 cJSON 数组 (调用方负责 cJSON_Delete), 失败返回 NULL
 */
cJSON* probe_targets_to_json(void);

/**
 * @brief 网络状态变化时提示一次 (相同状态不重复打印)
 *
 * 检测是每秒一次的, 直接打 INFO 会刷屏; 状态没变就什么都不打。
 * @param status 本次检测到的网络状态
 */
void log_network_state_change(network_status_t status);

/**
 * @brief 启动后台联网探测线程 (仅 Web 进程用)
 *
 * Web 线程是单线程事件循环, 现做一次探测会把同时轮询的接口一起拖超时,
 * 所以由后台线程定期探、/api/status/online 只读缓存。
 */
void net_probe_start(void);

/**
 * @brief 停止后台联网探测线程
 */
void net_probe_stop(void);

/**
 * @brief 读最近一次探测结果 (缓存, 立即返回)
 *
 * 线程刚起来还没探完时返回 STATUS_ERROR (前端会显示"未连接"), 下一轮轮询就正常了。
 * @return 网络状态
 */
network_status_t net_probe_status(void);

#endif
