#ifndef ESURFINGCLIENT_PLATFORMUTILS_H
#define ESURFINGCLIENT_PLATFORMUTILS_H

#include "States.h"

#include <inttypes.h>
#include <stdint.h>

#ifdef _WIN32

#define SEP '\\'

#else

#define SEP '/'

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#endif

#define XML_BUFFER_SIZE 1024
#define NAME_LENGTH 256

typedef enum
{
    GET_TICKET = 1,
    LOGIN = 2,
    HEART_BEAT = 3,
    TERM = 4
} XmlChoose;

typedef enum
{
    CONSOLE_FORMAT = 1,
    FILE_FORMAT = 2
} TimeFormat;

typedef struct
{
    uint8_t* data;
    size_t length;
} bytes_t;

/**
 * @brief 打包适配器数据
 * @return JSON 文本
 */
char* get_adapters_json();

/**
 * @brief 获取程序运行目录
 * @param dir_array 目录指针
 * @return 是否获取成功
 */
bool get_exec_dir(char* dir_array);

/**
 * @brief 获取程序可执行文件的完整路径
 *
 * 监管者 fork 之后要用它 exec 出子进程, 因此必须是不依赖 cwd 的绝对路径
 * @param path_array 路径缓冲 (至少 PATH_MAX 字节)
 * @return 是否获取成功
 */
bool get_exec_path(char* path_array);

/**
 * @brief 取配置文件的完整路径
 *
 * OpenWrt 上是 /etc/config/esurfingclient, 桌面分支是程序目录下的 ESurfingClient.json。
 * 注意桌面分支要等 load_cfg() 跑过才有值。
 * @return 配置文件的完整路径
 */
const char* get_config_path(void);

/**
 * @brief 记下启动时的父进程号
 *
 * 必须在程序一开始就调用: 父进程可能在启动后立刻就没了
 */
void record_parent_pid(void);

/**
 * @brief 父进程是否还在 (供被监管的子进程自查)
 *
 * 监管者被强杀时子进程不该留下来变成孤儿。判据是"当前父进程号是否还是启动时那个":
 * 不能简单地看是不是被过继给了 init (PID 1) —— 用 setsid 之类方式主动脱离终端的
 * 进程, 父进程本来就可能是 1, 那样会被误判成孤儿。
 *
 * Linux 上子进程另外登记了 PR_SET_PDEATHSIG, 父进程一死内核立刻发信号,
 * 这里只是兜底。
 * Windows 没有等价机制 (要彻底解决得用 Job Object), 恒返回 true。
 * @return 父进程是否还在
 */
bool parent_process_alive(void);

/**
 * @brief XML 解析
 * @param xml_data XML 数据
 * @param tag 提取标志
 * @return 解析后的数据
 */
char* xml_parser(const char* xml_data, const char* tag);

/**
 * @brief 文本转字节
 * @param str 文本数据
 * @return 字节数据
 */
bytes_t str2bytes(const char* str);

/**
 * @brief 字符串转换为 64 位长整型
 * @param str 要转换的字符串
 * @return 转换后的 64 位长整型
 */
uint64_t str2uint64(const char* str);

/**
 * @brief 64 位长整型转换为字符串
 * @param num 要转换的 64 位长整型
 * @return 转换后的字符串
 */
char* uint642str(uint64_t num);

/**
 * @brief 获取当前时间的毫秒时间戳
 * @return 64位时间戳
 */
uint64_t get_cur_tm_ms();

/**
 * @brief 获取随机字节
 * @param buf 缓冲
 * @param len 长度
 */
void get_rand_bytes(uint8_t* buf, size_t len);

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

/**
 * @brief 安全字符串
 * @param str 字符串
 * @return 过滤后的字符串
 */
const char* safe_str(const char* str);

/**
 * @brief 创建 XML 字符串
 * @param choose 格式化选择
 * @return XML 字符串
 */
char* create_xml_payload(XmlChoose choose);

/**
 * @brief 清除指定标签字段
 * @param text 需要清除的文本
 * @param start_tag 开始标签
 * @param end_tag 结束标签
 * @return 清除后的文本
 */
char* extract_between_tags(const char* text, const char* start_tag, const char* end_tag);

/**
 * @brief 清除 CDATA 字段
 * @param text 需要清除的文本
 * @return 清除后的文本
 */
char* clean_CDATA(const char* text);

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

#endif // ESURFINGCLIENT_PLATFORMUTILS_H
