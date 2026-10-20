#include "webserver/DashboardHelper.h"

#include "core/States.h"

#include "utils/log/Logger.h"
#include "utils/platform/PlatformUtils.h"

#ifdef _WIN32

#include <windows.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include "utils/sim/SimThread.h"

/** 发布包布局: <包根>\tools\windows-network-panel\ */
#define HELPER_DIR_PACKAGE "tools\\windows-network-panel"
/** 本地构建目录布局 (CMake 的 CopyWNP 目标复制到这里): <构建目录>\windows-network-panel\ */
#define HELPER_DIR_BUILD "windows-network-panel"
#define HELPER_SCRIPT "Start-Dashboard.ps1"
/** 热点控制桥的端口, 与脚本/README 里的默认值保持一致 */
#define HELPER_BRIDGE_PORT 18889
/** 等脚本自己收尾的时间: 实测 0.4 秒内就退出了, 3 秒足够宽松 */
#define HELPER_STOP_WAIT_MS 3000
/** 单行日志最多记这么多字符, 超出的截断 (脚本的输出不该撑爆日志缓冲) */
#define HELPER_LINE_MAX 1024
/** 拼路径的缓冲: 比 PATH_MAX 多留一段, 免得拼出来的路径被 snprintf 截断 */
#define HELPER_PATH_MAX (PATH_MAX + 64)
/** 停止信号文件的路径还要再长一点 (目录 + 文件名) */
#define HELPER_STOP_PATH_MAX (HELPER_PATH_MAX + 32)
/** 停止信号文件的名字 (放在日志目录里: 那里一定可写, 也不会被当成网页静态资源) */
#define HELPER_STOP_FILE "dashboard-%lu.stop"

typedef struct {
    HANDLE process;
    HANDLE pipe;                // 读端: 脚本的 stdout/stderr
    sim_thread_t* reader;       // 逐行转发日志的线程
    bool running;
    char stop_file[HELPER_STOP_PATH_MAX]; // UTF-8, 传给脚本与 CreateFileW/DeleteFileW
} helper_state_t;

static helper_state_t s_helper;

/**
 * @brief UTF-8 转 UTF-16
 *
 * 路径可能带中文 (Windows 的 ANSI 代码页不一定覆盖), 所以凡是要交给 Win32 的
 * 字符串都先转宽字符, 一律用 W 版接口
 */
static void helper_to_wide(const char* utf8, wchar_t* out, const size_t out_chars)
{
    if (out_chars == 0) return;
    out[0] = L'\0';
    if (utf8 == NULL || utf8[0] == '\0') return;
    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, out, (int)out_chars);
}

static bool helper_is_file(const char* path)
{
    const DWORD attr = GetFileAttributesA(path);
    return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

static bool helper_is_dir(const char* path)
{
    const DWORD attr = GetFileAttributesA(path);
    return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

/**
 * @brief 找到随包的 Dashboard 脚本
 *
 * 两种布局都要认: 发布包在 <包根>\tools\windows-network-panel\,
 * 本地构建目录在 <构建目录>\windows-network-panel\ (见 CMake 的 CopyWNP)。
 * 客户端旁边没有 portal\assets 时直接放弃 —— 脚本没地方写 traffic.json,
 * 拉起来也只会报错退出
 * @param script 输出的脚本绝对路径
 * @param len 缓冲长度
 * @return 是否找到
 */
static bool helper_locate_script(char* script, const size_t len)
{
    char dir[PATH_MAX];
    if (get_exec_dir(dir) == false) return false;

    char assets[HELPER_PATH_MAX];
    snprintf(assets, sizeof(assets), "%s%cportal%cassets", dir, SEP, SEP);
    if (helper_is_dir(assets) == false)
    {
        LOG_DEBUG("客户端旁边没有 portal/assets, 不启动 Dashboard 辅助脚本");
        return false;
    }

    const char* subdirs[] = {HELPER_DIR_PACKAGE, HELPER_DIR_BUILD};
    for (size_t i = 0; i < sizeof(subdirs) / sizeof(subdirs[0]); i++)
    {
        snprintf(script, len, "%s%c%s%c%s", dir, SEP, subdirs[i], SEP, HELPER_SCRIPT);
        if (helper_is_file(script)) return true;
    }
    return false;
}

/**
 * @brief 拼出停止信号文件的路径 (日志目录\dashboard-<本进程号>.stop)
 *
 * 带上进程号是为了不会被上一轮留下的文件误伤: 每次运行都是新名字,
 * 也就不用去猜哪个文件是"上次的"
 */
static void helper_build_stop_file(char* out, const size_t len)
{
    char base[HELPER_PATH_MAX];
    const char* log_dir = get_logger_dir();
    if (log_dir != NULL && log_dir[0] != '\0')
    {
        snprintf(base, sizeof(base), "%s", log_dir);
    }
    else if (get_exec_dir(base) == false)
    {
        base[0] = '.';
        base[1] = '\0';
    }

    // 去掉末尾的分隔符, 免得拼出 "logs\\dashboard-…" 这种双斜杠
    size_t n = strlen(base);
    while (n > 1 && (base[n - 1] == '\\' || base[n - 1] == '/')) base[--n] = '\0';

    snprintf(out, len, "%s%c" HELPER_STOP_FILE, base, SEP, (unsigned long)GetCurrentProcessId());
}

/**
 * @brief 日志转发线程: 把脚本的 stdout/stderr 一行一行写进本程序的日志
 *
 * 脚本那侧在输出被重定向时会切成 UTF-8 (没有控制台时用控制台代码页会把中文
 * 变成问号), 所以这里读到的就是 UTF-8, 直接原样记下来即可, 不用再转码
 */
static int helper_read_thread(void* arg)
{
    (void)arg;

    char chunk[256];
    char line[HELPER_LINE_MAX];
    size_t used = 0;
    DWORD read_bytes = 0;

    while (ReadFile(s_helper.pipe, chunk, (DWORD)sizeof(chunk), &read_bytes, NULL) && read_bytes > 0)
    {
        for (DWORD i = 0; i < read_bytes; i++)
        {
            if (chunk[i] == '\n')
            {
                while (used > 0 && line[used - 1] == '\r') used--;
                line[used] = '\0';
                if (used > 0) LOG_INFO("%s", line);
                used = 0;
            }
            else if (used + 1 < sizeof(line))
            {
                line[used++] = chunk[i];
            }
        }
    }

    // 最后一行可能没有换行
    while (used > 0 && line[used - 1] == '\r') used--;
    line[used] = '\0';
    if (used > 0) LOG_INFO("%s", line);
    return 0;
}

bool dashboard_helper_start(void)
{
    if (s_helper.running)
    {
        LOG_DEBUG("Dashboard 辅助脚本已在运行");
        return true;
    }

    char script[HELPER_PATH_MAX];
    if (helper_locate_script(script, sizeof(script)) == false)
    {
        LOG_DEBUG("没找到 %s (发布包或构建目录布局才会自动启动), 跳过 Dashboard 辅助脚本", HELPER_SCRIPT);
        return false;
    }

    char exec_path[HELPER_PATH_MAX];
    if (get_exec_path(exec_path) == false)
    {
        LOG_WARN("无法获取自身路径, 不启动 Dashboard 辅助脚本");
        return false;
    }

    /**
     * 一律用系统那套 Windows PowerShell 5.1: 脚本依赖 WinRT 投影,
     * PowerShell 7 上跑不起来 (与 Start-Dashboard.cmd 里写的一致)
     */
    char system_root[PATH_MAX];
    char powershell[HELPER_PATH_MAX];
    if (GetEnvironmentVariableA("SystemRoot", system_root, (DWORD)sizeof(system_root)) == 0)
    {
        LOG_WARN("读不到 SystemRoot, 不启动 Dashboard 辅助脚本");
        return false;
    }
    snprintf(powershell, sizeof(powershell), "%s\\System32\\WindowsPowerShell\\v1.0\\powershell.exe", system_root);
    if (helper_is_file(powershell) == false)
    {
        LOG_WARN("找不到 %s, 不启动 Dashboard 辅助脚本", powershell);
        return false;
    }

    helper_build_stop_file(s_helper.stop_file, sizeof(s_helper.stop_file));
    wchar_t w_stale[HELPER_PATH_MAX];
    helper_to_wide(s_helper.stop_file, w_stale, HELPER_PATH_MAX);
    DeleteFileW(w_stale); // 上一轮的残留不该让这一轮立刻收尾

    wchar_t w_powershell[HELPER_PATH_MAX];
    wchar_t w_script[HELPER_PATH_MAX];
    wchar_t w_exec[HELPER_PATH_MAX];
    wchar_t w_stop[HELPER_PATH_MAX];
    helper_to_wide(powershell, w_powershell, HELPER_PATH_MAX);
    helper_to_wide(script, w_script, HELPER_PATH_MAX);
    helper_to_wide(exec_path, w_exec, HELPER_PATH_MAX);
    helper_to_wide(s_helper.stop_file, w_stop, HELPER_PATH_MAX);

    /**
     * 参数与手工双击 Start-Dashboard.cmd 等价, 只多了 -StopFile:
     * -ClientPath 指自己 (脚本靠"进程名 + 路径"认出客户端), 不再由它去拉起客户端
     * -WebPort    用实际端口, 热点桥接的允许来源要跟网页对得上
     */
    wchar_t cmdline[HELPER_PATH_MAX * 5];
    const int used = swprintf(cmdline, sizeof(cmdline) / sizeof(cmdline[0]),
                              L"\"%ls\" -NoProfile -ExecutionPolicy Bypass -File \"%ls\""
                              L" -ClientPath \"%ls\" -WebPort %u -BridgePort %u -StopFile \"%ls\"",
                              w_powershell, w_script, w_exec, (unsigned)g_web_port,
                              (unsigned)HELPER_BRIDGE_PORT, w_stop);
    if (used <= 0 || (size_t)used >= sizeof(cmdline) / sizeof(cmdline[0]))
    {
        LOG_ERROR("Dashboard 辅助脚本的命令行过长, 跳过");
        return false;
    }

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    HANDLE pipe_read = NULL;
    HANDLE pipe_write = NULL;
    if (CreatePipe(&pipe_read, &pipe_write, &sa, 0) == 0)
    {
        LOG_WARN("创建管道失败 (错误码 %lu), 不启动 Dashboard 辅助脚本", (unsigned long)GetLastError());
        return false;
    }
    // 只让子进程继承写端: 读端留在本进程, 否则管道永远等不到 EOF
    SetHandleInformation(pipe_read, HANDLE_FLAG_INHERIT, 0);

    HANDLE nul_input = CreateFileA("NUL", GENERIC_READ | GENERIC_WRITE,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE, &sa,
                                   OPEN_EXISTING, 0, NULL);

    STARTUPINFOW si;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = pipe_write;
    si.hStdError = pipe_write;
    si.hStdInput = (nul_input == INVALID_HANDLE_VALUE) ? NULL : nul_input;

    PROCESS_INFORMATION pi;
    memset(&pi, 0, sizeof(pi));

    /**
     * CREATE_NO_WINDOW: 不在用户桌面上闪一个黑窗口 (脚本的输出由管道接走)
     * bInheritHandles 必须为 TRUE, 否则子进程拿不到管道
     */
    const BOOL ok = CreateProcessW(w_powershell, cmdline, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
    const DWORD create_error = ok ? 0 : GetLastError();

    if (nul_input != INVALID_HANDLE_VALUE) CloseHandle(nul_input);
    CloseHandle(pipe_write); // 本进程不再需要写端

    if (ok == 0)
    {
        CloseHandle(pipe_read);
        LOG_ERROR("启动 Dashboard 辅助脚本失败 (错误码 %lu)", (unsigned long)create_error);
        return false;
    }

    CloseHandle(pi.hThread);
    s_helper.process = pi.hProcess;
    s_helper.pipe = pipe_read;
    s_helper.reader = sim_thread_create(helper_read_thread, NULL);
    s_helper.running = true;

    LOG_INFO("Dashboard 辅助脚本已启动 (网页端口 %" PRIu16 ", 热点桥接 %d), 它的输出会记进本日志",
             g_web_port, HELPER_BRIDGE_PORT);
    return true;
}

void dashboard_helper_stop(void)
{
    if (s_helper.running == false) return;

    LOG_INFO("关闭 Dashboard 辅助脚本");

    /**
     * 先写停止信号, 让脚本自己收尾: 它看到文件就跳出采集循环,
     * 由 finally 停掉热点桥接、删临时文件、释放互斥锁
     */
    wchar_t w_stop[HELPER_PATH_MAX];
    helper_to_wide(s_helper.stop_file, w_stop, HELPER_PATH_MAX);
    const HANDLE marker = CreateFileW(w_stop, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                      NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (marker != INVALID_HANDLE_VALUE) CloseHandle(marker);

    bool forced = false;
    if (WaitForSingleObject(s_helper.process, HELPER_STOP_WAIT_MS) == WAIT_TIMEOUT)
    {
        LOG_WARN("Dashboard 辅助脚本 %d 毫秒内没有退出, 强制结束", HELPER_STOP_WAIT_MS);
        TerminateProcess(s_helper.process, 1);
        WaitForSingleObject(s_helper.process, 1000);
        forced = true;
    }

    /**
     * 子进程退出后管道的写端就全没了, 转发线程读到 EOF 自己结束。
     * 只有强杀失败这种极端情况才可能等不到 EOF, 那时先关读端把线程顶醒
     */
    if (forced)
    {
        if (WaitForSingleObject(s_helper.process, 0) == WAIT_TIMEOUT)
        {
            CloseHandle(s_helper.pipe);
            s_helper.pipe = NULL;
        }
    }
    if (s_helper.reader != NULL)
    {
        int code = 0;
        sim_thread_join(s_helper.reader, &code);
        s_helper.reader = NULL;
    }
    if (s_helper.pipe != NULL)
    {
        CloseHandle(s_helper.pipe);
        s_helper.pipe = NULL;
    }
    CloseHandle(s_helper.process);
    s_helper.process = NULL;

    DeleteFileW(w_stop); // 脚本自己也会删, 删两次没关系
    s_helper.running = false;
    LOG_INFO("Dashboard 辅助脚本已退出");
}

#else

bool dashboard_helper_start(void)
{
    return false;
}

void dashboard_helper_stop(void)
{
}

#endif
