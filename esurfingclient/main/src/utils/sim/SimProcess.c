#include "utils/sim/SimProcess.h"

#include "utils/PlatformUtils.h"
#include "utils/Logger.h"

#include "States.h"

#include <stdlib.h>
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#else
#include <unistd.h>
#endif

void restart_process()
{
    char exec_path[PATH_MAX];
    if (get_exec_path(exec_path) == false)
    {
        fprintf(stderr, "[ERROR] 无法获取自身可执行文件路径, 重启失败\n");
        exit(1);
    }

#ifdef _WIN32

    /**
     * 必须把原始参数一起带上:
     * 丢掉参数会让重启后的进程退化成另一个角色 (比如 --role auth 变成单进程模式)
     * 路径可能含空格, 所以要加引号
     */
    char cmdline[PATH_MAX + 256];
    int used = snprintf(cmdline, sizeof(cmdline), "\"%s\"", exec_path);
    for (int i = 1; i < g_main_argc && used > 0 && (size_t)used < sizeof(cmdline); i++)
    {
        used += snprintf(cmdline + used, sizeof(cmdline) - (size_t)used, " %s", g_main_argv[i]);
    }

    STARTUPINFO si = {0};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {0};
    CreateProcess(exec_path, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    ExitProcess(0);

#else

    char* argv[64];
    int n = 0;

    argv[n++] = exec_path;
    for (int i = 1; i < g_main_argc && n < 63; i++)
    {
        argv[n++] = g_main_argv[i];
    }
    argv[n] = NULL;

    execv(exec_path, argv);
    perror("execv");
    exit(1);

#endif
}
