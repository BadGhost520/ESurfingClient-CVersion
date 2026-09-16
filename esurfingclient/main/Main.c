#include "utils/PlatformUtils.h"
#include "utils/Service.h"
#include "control/Control.h"

#include "States.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

extern void work(void);

/** @brief 参数解析结果, 该值表示继续运行, 其它值直接作为退出码返回 */
#define ARG_CONTINUE (-1)

/** @brief 是否只列举可用账号 */
static bool s_list_accounts = false;

/**
 * @brief 显示帮助信息
 */
static void PrintUsage()
{
    printf("使用格式: ESurfingClient [选项]\n");
    printf("  [nothing]            直接运行程序 (前台模式)\n");
    printf("  -r, --role <角色>     程序角色: supervisor (监管) / auth (认证) / web (网页)\n");
    printf("  -a, --account <序号>  指定本进程负责的配置序号 (从 1 开始, auth 角色必填)\n");
    printf("  --list-accounts      列出配置文件中所有可用账号的序号后退出 (供 init 脚本使用)\n");
#ifndef __OPENWRT__
    printf("  --control-port <端口> 控制通道端口 (默认 %d; 认证进程监听, Web 进程连接)\n", CONTROL_DEFAULT_PORT);
    printf("  --web-listen <地址>   Web 服务监听地址 (默认 %s)\n", DEFAULT_WEB_LISTEN);
#endif
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

#ifndef __OPENWRT__

/**
 * @brief 解析端口 (1 - 65535)
 * @param str 端口文本
 * @param port 解析结果
 * @return 是否解析成功
 */
static bool parse_port(const char* str, uint16_t* port)
{
    char* end = NULL;
    const long value = strtol(str, &end, 10);

    if (end == NULL || end == str || *end != '\0') return false;
    if (value < 1 || value > 65535) return false;

    *port = (uint16_t)value;
    return true;
}

/**
 * @brief 校验监听地址是否合法 (形如 127.0.0.1:8888)
 * @param str 地址文本
 * @return 是否合法
 */
static bool check_listen_addr(const char* str)
{
    if (str == NULL || str[0] == '\0') return false;
    if (strlen(str) >= WEB_LISTEN_LEN) return false;

    const char* colon = strrchr(str, ':');
    if (colon == NULL || colon == str) return false;

    uint16_t port = 0;
    return parse_port(colon + 1, &port);
}

#endif  // !__OPENWRT__ (控制通道与 Web 监听地址只存在于非 OpenWrt 构建)

/**
 * @brief 校验角色与序号的组合是否合法
 * @return 0 合法, 其它值作为退出码返回
 */
static int check_args()
{
    /**
     * 列举账号是一个独立的查询动作, 由 init 脚本调用,
     * 与角色/序号组合在一起语义不清, 直接拒绝
     */
    if (s_list_accounts == true)
    {
        if (g_prog_role != ROLE_STANDALONE)
        {
            fprintf(stderr, "[ERROR] --list-accounts 不能与 --role 一起使用\n");
            return 1;
        }
        if (g_prog_account != 0)
        {
            fprintf(stderr, "[ERROR] --list-accounts 不能与 --account 一起使用\n");
            return 1;
        }
        return 0;
    }

    /**
     * 监管进程与 Web 进程都只存在于非 OpenWrt 构建:
     * OpenWrt 上由 procd 每账号起一个实例, 不需要监管者
     */
#ifdef __OPENWRT__

    if (g_prog_role == ROLE_WEB || g_prog_role == ROLE_SUPERVISOR)
    {
        fprintf(stderr, "[ERROR] OpenWRT 版本不包含 Web 服务与监管进程, 只能用 auth 角色\n");
        return 1;
    }

#endif

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

        if (strcmp(arg, "--list-accounts") == 0)
        {
            s_list_accounts = true;
            continue;
        }

#ifndef __OPENWRT__

        if (strcmp(arg, "--control-port") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf(stderr, "[ERROR] %s 缺少端口\n", arg);
                PrintUsage();
                return 1;
            }
            if (parse_port(argv[++i], &g_control_port) == false)
            {
                fprintf(stderr, "[ERROR] 控制端口无效 (应为 1 - 65535): %s\n", argv[i]);
                PrintUsage();
                return 1;
            }
            continue;
        }

        if (strcmp(arg, "--web-listen") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf(stderr, "[ERROR] %s 缺少监听地址\n", arg);
                PrintUsage();
                return 1;
            }
            if (check_listen_addr(argv[++i]) == false)
            {
                fprintf(stderr, "[ERROR] 监听地址无效 (应形如 127.0.0.1:8888): %s\n", argv[i]);
                PrintUsage();
                return 1;
            }
            snprintf(g_web_listen, sizeof(g_web_listen), "%s", argv[i]);
            continue;
        }

#endif

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

    // 记下来: 重启进程时要原样带上这些参数, 否则重启后会变成另一个角色
    g_main_argc = argc;
    g_main_argv = argv;

#ifdef _WIN32

    system("chcp 65001 >nul");

#endif

    const int arg_result = parse_args(argc, argv);
    if (arg_result != ARG_CONTINUE)
    {
        return arg_result;
    }

    if (s_list_accounts == true)
    {
        return list_accounts() < 0 ? 1 : 0;
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
