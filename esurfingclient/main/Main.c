#include "utils/PlatformUtils.h"
#include "utils/Service.h"

#include "States.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

extern void work(void);

/** @brief 参数解析结果, 该值表示继续运行, 其它值直接作为退出码返回 */
#define ARG_CONTINUE (-1)

/**
 * @brief 显示帮助信息
 */
static void PrintUsage()
{
    printf("使用格式: ESurfingClient [选项]\n");
    printf("  [nothing]            直接运行程序 (前台模式)\n");
    printf("  -r, --role <角色>     指定程序角色: auth (认证进程)\n");
    printf("                       (supervisor 守护进程与 web 网页进程尚未实现)\n");
    printf("  -a, --account <序号>  指定本进程负责的配置序号 (从 1 开始, auth 角色必填)\n");
#if !defined(__OPENWRT__) && !defined(__ANDROID__)
    printf("  -i, --install        安装为系统服务 (需要管理员/root 权限)\n");
    printf("  -u, --uninstall      卸载系统服务 (需要管理员/root 权限)\n");
#endif
    printf("  -h, --help           显示此帮助信息\n");
}

/**
 * @brief 解析角色名称
 * @param name 角色名称
 * @param role 解析结果
 * @return 是否解析成功
 */
static bool parse_role(const char* name, prog_role_t* role)
{
    if (strcmp(name, "supervisor") == 0)
    {
        *role = ROLE_SUPERVISOR;
        return true;
    }
    if (strcmp(name, "auth") == 0)
    {
        *role = ROLE_AUTH;
        return true;
    }
    if (strcmp(name, "web") == 0)
    {
        *role = ROLE_WEB;
        return true;
    }
    return false;
}

/**
 * @brief 解析配置序号 (1 - 255)
 * @param str 序号文本
 * @param idx 解析结果
 * @return 是否解析成功
 */
static bool parse_account(const char* str, uint8_t* idx)
{
    char* end = NULL;
    const long value = strtol(str, &end, 10);

    if (end == NULL || end == str || *end != '\0') return false;
    if (value < 1 || value > UINT8_MAX) return false;

    *idx = (uint8_t)value;
    return true;
}

/**
 * @brief 校验角色与序号的组合是否合法
 * @return 0 合法, 其它值作为退出码返回
 */
static int check_args()
{
    /**
     * 守护进程与 Web 进程尚未实现.
     * 这里直接拒绝, 而不是悄悄按单进程模式跑 —— 否则使用者会误以为进程已经拆开了
     */
    if (g_prog_role == ROLE_SUPERVISOR || g_prog_role == ROLE_WEB)
    {
        fprintf(stderr, "[ERROR] 角色 %s 尚未实现, 目前只支持 auth\n",
            g_prog_role == ROLE_SUPERVISOR ? "supervisor" : "web");
        return 1;
    }

    if (g_prog_role == ROLE_AUTH && g_prog_account == 0)
    {
        fprintf(stderr, "[ERROR] --role auth 必须配合 --account 指定负责的配置序号\n");
        return 1;
    }

    if (g_prog_account != 0 && g_prog_role != ROLE_AUTH && g_prog_role != ROLE_STANDALONE)
    {
        fprintf(stderr, "[ERROR] --account 只能配合 --role auth 使用\n");
        return 1;
    }

    return 0;
}

/**
 * @brief 解析命令行参数
 * @return ARG_CONTINUE 表示继续运行, 其它值作为退出码返回
 */
static int parse_args(const int argc, char* argv[])
{
    for (int i = 1; i < argc; i++)
    {
        const char* arg = argv[i];

        if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0)
        {
            PrintUsage();
            return 0;
        }

#if !defined(__OPENWRT__) && !defined(__ANDROID__)

        if (strcmp(arg, "--install") == 0 || strcmp(arg, "-i") == 0)
        {
            return service_install();
        }
        if (strcmp(arg, "--uninstall") == 0 || strcmp(arg, "-u") == 0)
        {
            return service_uninstall();
        }

#endif

        if (strcmp(arg, "--role") == 0 || strcmp(arg, "-r") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf(stderr, "[ERROR] %s 缺少角色名称\n", arg);
                PrintUsage();
                return 1;
            }
            if (parse_role(argv[++i], &g_prog_role) == false)
            {
                fprintf(stderr, "[ERROR] 未知角色: %s\n", argv[i]);
                PrintUsage();
                return 1;
            }
            continue;
        }

        if (strcmp(arg, "--account") == 0 || strcmp(arg, "-a") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf(stderr, "[ERROR] %s 缺少配置序号\n", arg);
                PrintUsage();
                return 1;
            }
            if (parse_account(argv[++i], &g_prog_account) == false)
            {
                fprintf(stderr, "[ERROR] 配置序号无效 (应为 1 - 255): %s\n", argv[i]);
                PrintUsage();
                return 1;
            }
            continue;
        }

        fprintf(stderr, "[ERROR] 未知参数: %s\n", arg);
        PrintUsage();
        return 1;
    }

    const int check_result = check_args();
    if (check_result != 0) return check_result;

    return ARG_CONTINUE;
}

int main(const int argc, char *argv[])
{
    g_start_run_tm = get_cur_tm_ms(); // 获取开始运行的时间

#ifdef _WIN32

    system("chcp 65001 >nul");

#endif

    const int arg_result = parse_args(argc, argv);
    if (arg_result != ARG_CONTINUE)
    {
        return arg_result;
    }

#ifdef _WIN32

    const SERVICE_TABLE_ENTRY ServiceTable[] = {
        {.lpServiceName = SERVICE_NAME, .lpServiceProc = (LPSERVICE_MAIN_FUNCTION)ServiceMain},
        {.lpServiceName = NULL, .lpServiceProc = NULL}
    };
    if (StartServiceCtrlDispatcher(ServiceTable))
    {
        return 0;
    }

#endif

    work();

    return 0;
}
