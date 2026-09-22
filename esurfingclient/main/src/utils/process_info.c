#include "utils/PlatformUtils.h"

#ifndef _WIN32
/** @brief 启动时的父进程号 (用来判断父进程是否已经没了) */
static pid_t s_parent_pid = 0;
#endif

void record_parent_pid(void)
{
#ifndef _WIN32
    s_parent_pid = getppid();
#endif
}

bool parent_process_alive(void)
{
#ifdef _WIN32
    /* Windows 没有等价机制 (要彻底解决得用 Job Object) */
    return true;
#else
    /* 没登记过就不做判断, 免得误杀 */
    if (s_parent_pid == 0) return true;

    /*
     * 关键: 是"和启动时那个父进程号比", 而不是"看是不是 1"。
     * 用 setsid 之类方式主动脱离终端的进程, 父进程本来就可能是 init,
     * 按"是不是 1"判断会把它误当成孤儿
     */
    return getppid() == s_parent_pid;
#endif
}
