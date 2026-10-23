#include "utils/sim/SimProcess.h"

#include "core/States.h"

#include "utils/platform/PlatformUtils.h"
#include "utils/log/Logger.h"

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#else
#include <unistd.h>
#include <stdlib.h>
#endif

void restart_process(void)
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
     * 丢掉参数会让重启后的进程退化成另一个角色 (比如 --role auth 变成守护进程)
     */
    char cmdline[PATH_MAX + 256];
    if (build_windows_cmdline(exec_path, g_main_argv, cmdline, sizeof(cmdline)) == false)
    {
        fprintf(stderr, "[ERROR] 重启命令行过长, 重启失败\n");
        exit(1);
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
