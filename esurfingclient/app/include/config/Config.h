#ifndef ESURFINGCLIENT_CONFIG_H
#define ESURFINGCLIENT_CONFIG_H

#include <stdbool.h>
#include <stdint.h>

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
 *
 * 配置不可用 (程序被 enabled 关掉 / 账号没填全 / JSON 写坏 / 文件不存在) 时返回 false,
 * 但不会挂住也不会退出: 调用方带着 Web 服务停下来等用户把配置改好。
 *
 * 只要 JSON 解析得动, 能读出来的值都会照常填进内存 (包括账号密码与 Web 服务端口),
 * 配置页读的就是这块内存 —— 不然用户一保存就把自己写过的东西覆盖成空的了
 */
bool load_cfg(void);

/**
 * @brief 读取配置文件的原文
 *
 * 程序还没把配置读起来时 (停在"等配置"状态, 或者刚保存、重新加载还没跑完),
 * 网页直接展示文件里的内容 —— 内存里那份是空的或者旧的
 * @return 文件内容 (需要 free), 读不到或文件为空时返回 NULL
 */
char* read_cfg_file(void);

/**
 * @brief 取配置文件内容的指纹 (用于判断它有没有被改过)
 *
 * 等配置的地方靠它决定要不要重新解析: 配置没变就反复解析只会把日志刷满。
 * 看的是内容而不是修改时间, 见 Config.c 里的说明
 * @return 指纹值 (文件不存在或读不到时为 0)
 */
uint64_t cfg_file_hash(void);

/**
 * @brief 等配置文件变成可用 (每次最多等一个固定的间隔)
 *
 * 供配置不可用时的"等配置"状态使用: 网页上的"保存并应用"会置 g_need_reload,
 * 手工编辑配置文件则靠上面的指纹发现, 两者都能让本函数立刻重新解析一遍
 * @return true 表示配置已可用 (此时 load_cfg 已经跑完)
 */
bool wait_cfg_usable(void);

/**
 * @brief 列举配置文件中所有可用账号的序号
 *
 * 供 OpenWrt 的 init 脚本使用: 每个可用账号起一个认证进程实例。
 * 复用 load_cfg 的校验逻辑, 结果与真正会被加载的账号完全一致
 * @return 账号数量, -1 表示配置有问题
 */
int list_accounts(void);

/**
 * @brief 获取配置文件路径
 * @return 配置文件路径
 */
const char* get_config_file_path(void);

/** 通道序号 → 名字 (1 windows / 2 linux / 3 android / 4 ios / 5 macos) */
const char* channel_name(uint8_t chn);

/**
 * @brief 通道序号 → 认证报文里的操作系统标识
 *
 * 每个通道用当前最主流的系统版本, 硬编码在通道表里 (Windows 11 / Linux /
 * Android 14 / iOS 17.0 / macOS 14.4), 不再随机生成。
 */
const char* channel_ostag(uint8_t chn);

#endif
