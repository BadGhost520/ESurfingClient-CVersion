#include "utils/platform/PlatformUtils.h"

#include <string.h>

#ifdef _WIN32

#include <windows.h>
#include <tlhelp32.h>

static unsigned long s_parent_pid = 0;

#else

#include <unistd.h>

static pid_t s_parent_pid = 0;

#endif

#ifdef _WIN32

/**
 * @brief 用一次进程快照查出自己的父进程号
 *
 * 只在启动时跑一次, 开销可以忽略。失败返回 0, 调用方据此放弃判断 ——
 * 宁可留下孤儿也不能误杀: 父进程号取错的话, 子进程会在启动后立刻判定
 * "监护人没了"并退出。
 * @return 父进程号, 取不到为 0
 */
static unsigned long find_parent_pid(void)
{
    const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32 entry;
    memset(&entry, 0, sizeof(entry));
    entry.dwSize = sizeof(entry);

    const unsigned long self = (unsigned long)GetCurrentProcessId();
    unsigned long parent = 0;

    if (Process32First(snapshot, &entry) == TRUE)
    {
        do
        {
            if ((unsigned long)entry.th32ProcessID == self)
            {
                parent = (unsigned long)entry.th32ParentProcessID;
                break;
            }
            entry.dwSize = sizeof(entry);
        } while (Process32Next(snapshot, &entry) == TRUE);
    }

    CloseHandle(snapshot);
    return parent;
}

#endif

void record_parent_pid(void)
{
#ifdef _WIN32
    s_parent_pid = find_parent_pid();
#else
    s_parent_pid = getppid();
#endif
}

bool parent_process_alive(void)
{
#ifdef _WIN32

    /* 没取到父进程号就不做判断, 免得误杀 */
    if (s_parent_pid == 0) return true;

    /**
     * SYNCHRONIZE 的权限要求最小。
     * 打不开句柄 = 进程已经不在了 (父子由同一个用户启动, 权限一致, 失败基本只有
     * 这一种); WAIT_TIMEOUT = 还活着; 其它返回值按"已退出"处理 —— 监管者不在了,
     * 本进程迟早要被清理, 早点自己走还能跑一遍登出。
     */
    const HANDLE handle = OpenProcess(SYNCHRONIZE, FALSE, (DWORD)s_parent_pid);
    if (handle == NULL) return false;

    const DWORD result = WaitForSingleObject(handle, 0);
    CloseHandle(handle);

    return result == WAIT_TIMEOUT;

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
