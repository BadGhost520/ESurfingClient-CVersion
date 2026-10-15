#ifndef ESURFINGCLIENT_PLATFORMUTILS_H
#define ESURFINGCLIENT_PLATFORMUTILS_H

#include <stdbool.h>
#include <stddef.h>

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

/* 平台相关: 程序自身的位置、父进程、本机网络适配器 */

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
