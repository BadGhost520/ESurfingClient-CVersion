#ifndef ESURFINGCLIENT_PLATFORMUTILS_H
#define ESURFINGCLIENT_PLATFORMUTILS_H

#include <stdbool.h>
#include <stddef.h>
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

#define NAME_LENGTH 256

#ifndef __OPENWRT__
/** 网卡标识的最大长度 (Windows 上是 GUID 字符串, 其它平台是接口名) */
#define ADAPTER_ID_LEN 64

/* 平台相关: 程序自身的位置、父进程、本机网络适配器 */

/**
 * 网卡流量统计的一帧
 *
 * 计数器是"自开机以来的累计字节数", 速率由调用方按两次采样的差值算;
 * 界面上的网速卡片就是这么来的 (每秒采一次, 差分后变成 B/s)
 */
typedef struct
{
    char id[ADAPTER_ID_LEN];  // Windows: 网卡 GUID (小写, 不带花括号); 其它平台: 接口名
    char name[NAME_LENGTH];   // 显示名
    bool connected;           // 是否已连接 (未连接的也列出来, 界面上标"未连接")
    uint64_t rx_bytes;        // 累计接收字节数
    uint64_t tx_bytes;        // 累计发送字节数
} adapter_stats_t;
#endif

/**
 * @brief 采集各网卡的累计收发字节数, 以及当前默认出口网卡
 *
 * 与 get_adapters() 的区别: 那个只给"名称 + IP"给配置页挑网卡用,
 * 这个多带计数器, 给网页上的实时网速用。列的是"物理网卡" (有线 / Wi-Fi / 蜂窝),
 * 未连接的也照样返回 (connected=false), 回环、隧道、VMware/Hyper-V/蓝牙这类
 * 软件网卡不算在内。
 *
 * Windows 上走 GetAdaptersAddresses + GetIfEntry2; 其它平台暂未实现 (返回 0)。
 *
 * @param out 输出数组
 * @param max 数组容量
 * @param default_id 默认出口网卡的 id (可为 NULL; 取不到时写空字符串)
 * @param default_id_len default_id 的缓冲长度
 * @return 采集到的网卡数量
 */
#ifndef __OPENWRT__
uint8_t get_adapters_stats(adapter_stats_t* out, uint8_t max, char* default_id, size_t default_id_len);
#endif

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

#endif
