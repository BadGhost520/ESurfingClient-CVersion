#ifndef ESURFINGCLIENT_CONFIG_H
#define ESURFINGCLIENT_CONFIG_H

#include <stdbool.h>

/**
 * @brief 取配置文件的完整路径
 *
 * OpenWrt 上是 /etc/config/esurfingclient, 桌面分支是程序目录下的 ESurfingClient.json。
 * 注意桌面分支要等 load_cfg() 跑过才有值。
 * @return 配置文件的完整路径
 */
const char* get_config_path(void);

/**
 * @brief 保存配置文件
 * @param configs_str 配置文件字符串
 */
bool save_cfg(const char* configs_str);

/**
 * @brief 加载配置文件
 */
bool load_cfg();

/**
 * @brief 列举配置文件中所有可用账号的序号
 *
 * 供 OpenWrt 的 init 脚本使用: 每个可用账号起一个认证进程实例。
 * 复用 load_cfg 的校验逻辑, 结果与真正会被加载的账号完全一致
 * @return 账号数量, -1 表示配置有问题
 */
int list_accounts();

/**
 * @brief 获取配置文件路径
 * @return 配置文件路径
 */
const char* get_config_file_path(void);

#endif //ESURFINGCLIENT_CONFIG_H
