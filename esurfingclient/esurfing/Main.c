#include "utils/PlatformUtils.h"
#include "utils/Service.h"
#include "States.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

extern void work(void);

#ifndef __OPENWRT__

static void PrintUsage()
{
    printf("参数:\n");
    printf("  [nothing]     直接运行程序 (前台模式)\n");
    printf("  --install     安装为系统服务 (需要管理员/root 权限)\n");
    printf("  --uninstall   卸载系统服务 (需要管理员/root 权限)\n");
    printf("  --help        显示此帮助信息\n");
}

#endif

int main(int argc, char *argv[])
{
    g_start_run_tm = get_cur_tm_ms(); // 获取开始运行的时间

#ifdef _WIN32

    system("chcp 65001 >nul");

#endif

#ifndef __OPENWRT__

    if (argc > 1)
    {
        if (strcmp(argv[1], "--install") == 0 || strcmp(argv[1], "-i") == 0)
        {
            return service_install();
        }
        if (strcmp(argv[1], "--uninstall") == 0 || strcmp(argv[1], "-u") == 0)
        {
            return service_uninstall();
        }
        if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)
        {
            PrintUsage();
            return 0;
        }
        fprintf(stderr, "[ERROR] 未知参数: %s\n", argv[1]);
        PrintUsage();
        return 1;
    }

#endif

#ifdef _WIN32

    SERVICE_TABLE_ENTRY ServiceTable[] = {
        {SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)ServiceMain},
        {NULL, NULL}
    };
    if (StartServiceCtrlDispatcher(ServiceTable))
    {
        return 0;
    }

#endif

    work();

    return 0;
}
