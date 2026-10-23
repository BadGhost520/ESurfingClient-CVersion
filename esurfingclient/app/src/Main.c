#include "config/Config.h"
#include "control/Control.h"
#include "core/States.h"
#include "utils/log/Logger.h"
#include "utils/platform/PlatformUtils.h"
#include "utils/time/TimeUtils.h"
#include "utils/runtime/Service.h"

#include <curl/curl.h>

#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <string.h>
#include <stdlib.h>
#endif

extern void work(void);

#define ARG_CONTINUE (-1)

static bool s_list_accounts = false;

static bool s_print_log_dir = false;

#ifdef _WIN32
static UINT s_console_output_cp = 0;
static UINT s_console_input_cp = 0;

/**
 * @brief 退出时把控制台代码页切回原来的值
 *
 * 切了不恢复的话, 用户之后在同一个窗口里跑别的程序 (期待 GBK) 反而会乱码。
 */
static void console_utf8_restore(void)
{
    if (s_console_output_cp != 0) SetConsoleOutputCP(s_console_output_cp);
    if (s_console_input_cp != 0) SetConsoleCP(s_console_input_cp);
}

/**
 * @brief 把控制台切到 UTF-8
 *
 * 程序里的中文是 UTF-8 编码的字节, 而 Windows 控制台默认按 GBK(936) 解释 ——
 * 不切代码页的话, 终端里所有中文都是乱码。服务模式没有控制台,
 * SetConsoleOutputCP 会失败, 此时不注册恢复函数即可。
 */
static void console_utf8_init(void)
{
    s_console_output_cp = GetConsoleOutputCP();
    s_console_input_cp = GetConsoleCP();

    if (SetConsoleOutputCP(CP_UTF8) != 0)
    {
        atexit(console_utf8_restore);
    }
    SetConsoleCP(CP_UTF8);
}
#endif

/**
 * @brief 显示帮助信息
 */
static void PrintUsage(void)
{
    printf("使用格式: ESurfingClient [选项]\n");
#ifdef __OPENWRT__
    printf("  [nothing]            直接运行程序 (单实例: 一个进程带所有账号的认证线程)\n");
    printf("  -r, --role <角色>     程序角色: auth (认证; 由 init 脚本按账号各起一个)\n");
#else
    printf("  [nothing]            直接运行程序 (等同于 --role supervisor: 认证与 Web 各起独立进程)\n");
    printf("  -r, --role <角色>     程序角色: supervisor (守护) / auth (认证) / web (网页)\n");
#endif
    printf("  -a, --account <序号>  指定本进程负责的配置序号 (从 1 开始, auth 角色必填)\n");
    printf("  --list-accounts      列出配置文件中所有可用账号的序号后退出 (供 init 脚本使用)\n");
    printf("  --print-log-dir      打印实际使用的日志目录后退出 (日志目录由配置里的 log_dir 决定)\n");
#ifndef __OPENWRT__
    printf("  --control-port <端口> 控制通道端口 (默认 %d; 被占用时自动往后顺延)\n", CONTROL_DEFAULT_PORT);
    printf("  --control-token <令牌> 控制通道令牌 (不填则不校验; 守护进程会自动生成并下发)\n");
    printf("  (Web 服务端口与是否允许外部访问在配置文件的 web_port / web_external_acc 里改)\n");
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

#endif  // !__OPENWRT__ (控制通道只存在于非 OpenWrt 构建)

/**
 * @brief 校验角色与序号的组合是否合法
 * @return 0 合法, 其它值作为退出码返回
 */
static int check_args(void)
{
    /**
     * 列举账号与打印日志目录都是独立的查询动作, 由 init 脚本调用,
     * 与角色/序号组合在一起语义不清, 直接拒绝
     */
    if (s_list_accounts == true || s_print_log_dir == true)
    {
        const char* self = s_list_accounts ? "--list-accounts" : "--print-log-dir";

        if (g_prog_role != ROLE_STANDALONE)
        {
            fprintf(stderr, "[ERROR] %s 不能与 --role 一起使用\n", self);
            return 1;
        }
        if (g_prog_account != 0)
        {
            fprintf(stderr, "[ERROR] %s 不能与 --account 一起使用\n", self);
            return 1;
        }
        if (s_list_accounts == true && s_print_log_dir == true)
        {
            fprintf(stderr, "[ERROR] --list-accounts 不能与 --print-log-dir 一起使用\n");
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
        fprintf(stderr, "[ERROR] OpenWRT 版本不包含 Web 服务与守护进程, 只能用 auth 角色\n");
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

        if (strcmp(arg, "--print-log-dir") == 0)
        {
            s_print_log_dir = true;
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

        if (strcmp(arg, "--control-token") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf(stderr, "[ERROR] %s 缺少令牌\n", arg);
                PrintUsage();
                return 1;
            }
            if (argv[i + 1][0] == '\0' || strlen(argv[i + 1]) >= CONTROL_TOKEN_LEN)
            {
                fprintf(stderr, "[ERROR] 令牌长度无效 (应为 1 - %d 个字符)\n", CONTROL_TOKEN_LEN - 1);
                PrintUsage();
                return 1;
            }
            snprintf(g_control_token, sizeof(g_control_token), "%s", argv[++i]);
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

/**
 * @brief 从环境变量补上控制通道令牌
 *
 * POSIX 上监管者是用环境变量下发令牌的, 不走命令行 ——
 * /proc/<PID>/cmdline 是全局可读的, 令牌放那里同机其它用户 ps 一下就看到了;
 * 环境变量对应的 /proc/<PID>/environ 只有属主和 root 可读。
 *
 * 显式给的 --control-token 优先, 这里只在没给的时候兜底。
 * (Windows 上监管者仍然走命令行, 原因见 SupervisorChild.c 的 child_build_argv)
 */
static void load_control_token_env(void)
{
    if (g_control_token[0] != '\0') return;

    const char* env_token = getenv(CONTROL_TOKEN_ENV);
    if (env_token == NULL || env_token[0] == '\0') return;

    if (strlen(env_token) >= CONTROL_TOKEN_LEN)
    {
        fprintf(stderr, "[ERROR] 环境变量 %s 过长, 已忽略\n", CONTROL_TOKEN_ENV);
        return;
    }

    snprintf(g_control_token, sizeof(g_control_token), "%s", env_token);
}

int main(const int argc, char *argv[])
{
#ifdef _WIN32
    console_utf8_init(); // 让控制台按 UTF-8 显示中文 (退出时会切回原代码页)
#endif

    g_start_run_tm = get_cur_tm_ms(); // 获取开始运行的时间

    /**
     * libcurl 的硬性要求: 多线程程序里全局初始化必须在创建任何线程之前做一次。
     * 否则每个线程第一次 curl_easy_init 各自触发的隐式初始化存在竞态
     * (可能拿到还没初始化完的解析器/SSL 状态)。本程序会起认证/Web/网速等多个线程。
     */
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK)
    {
        fprintf(stderr, "[ERROR] libcurl 全局初始化失败\n");
        return 1;
    }

    // 记下来: 重启进程时要原样带上这些参数, 否则重启后会变成另一个角色
    g_main_argc = argc;
    g_main_argv = argv;

    // 记下启动时的父进程号: 被监管的子进程靠它判断监管者是否还活着
    record_parent_pid();

    const int arg_result = parse_args(argc, argv);
    if (arg_result != ARG_CONTINUE)
    {
        return arg_result;
    }

    // 令牌也可以由监管者通过环境变量下发 (不暴露在命令行里)
    load_control_token_env();

    if (s_list_accounts == true)
    {
        return list_accounts() < 0 ? 1 : 0;
    }

    if (s_print_log_dir == true)
    {
        const char* log_dir = print_log_dir();
        if (log_dir == NULL) return 1;

        printf("%s\n", log_dir);
        fflush(stdout);
        return 0;
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
