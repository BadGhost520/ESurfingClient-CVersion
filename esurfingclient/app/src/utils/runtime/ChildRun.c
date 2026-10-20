#include "utils/runtime/ChildRun.h"

#include "utils/log/Logger.h"

#ifdef _WIN32

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

/**
 * 命令行缓冲 (宽字符个数)
 *
 * CreateProcessW 的命令行上限是 32767 个字符; 热点那边会塞一个十几 KB 的
 * -EncodedCommand, 所以这里按上限留足, 别拿 PATH_MAX 去卡它
 */
#define CHILD_CMDLINE_MAX (PATH_MAX * 64)
/** 轮询间隔: 既要及时读走输出, 又不要空转 */
#define CHILD_POLL_MS 10

/**
 * @brief UTF-8 转 UTF-16
 *
 * 路径与参数可能带中文, 统一走 W 版接口
 */
static bool child_to_wide(const char* utf8, wchar_t* out, const size_t out_chars)
{
    if (out_chars == 0) return false;
    out[0] = L'\0';
    if (utf8 == NULL || utf8[0] == '\0') return false;
    // 转不下就是真错误 (会被上层记下来), 不能静默当成空参数
    return MultiByteToWideChar(CP_UTF8, 0, utf8, -1, out, (int)out_chars) > 0;
}

/**
 * @brief 把还留在管道里的数据读干净 (不等待)
 * @return 是否读到数据
 */
static bool child_drain(HANDLE pipe, char* out, const size_t out_len, size_t* used)
{
    DWORD available = 0;
    if (PeekNamedPipe(pipe, NULL, 0, NULL, &available, NULL) == 0 || available == 0) return false;

    char chunk[256];
    while (available > 0)
    {
        DWORD read_bytes = 0;
        const DWORD want = available > sizeof(chunk) ? (DWORD)sizeof(chunk) : available;
        if (ReadFile(pipe, chunk, want, &read_bytes, NULL) == 0 || read_bytes == 0) break;
        for (DWORD i = 0; i < read_bytes && *used + 1 < out_len; i++)
        {
            out[(*used)++] = chunk[i];
        }
        available -= read_bytes;
        if (*used + 1 >= out_len) break;
    }
    out[*used] = '\0';
    return true;
}

bool child_run_capture(const char* exe, const char* args, char* out, const size_t out_len,
                       const int timeout_ms, int* exit_code, const bool merge_stderr)
{
    if (exe == NULL || out == NULL || out_len == 0) return false;
    out[0] = '\0';
    if (exit_code != NULL) *exit_code = -1;

    wchar_t w_exe[PATH_MAX];
    if (child_to_wide(exe, w_exe, PATH_MAX) == false)
    {
        LOG_WARN("子进程路径转换失败: %s", exe);
        return false;
    }

    wchar_t w_cmdline[CHILD_CMDLINE_MAX];
    if (child_to_wide(exe, w_cmdline, PATH_MAX) == false) return false;
    if (args != NULL && args[0] != '\0')
    {
        const size_t used = wcslen(w_cmdline);
        w_cmdline[used] = L' ';
        if (child_to_wide(args, w_cmdline + used + 1, CHILD_CMDLINE_MAX - used - 1) == false)
        {
            LOG_WARN("子进程命令行过长, 转换失败 (参数 %d 字节)", (int)strlen(args));
            return false;
        }
    }

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    HANDLE pipe_read = NULL;
    HANDLE pipe_write = NULL;
    if (CreatePipe(&pipe_read, &pipe_write, &sa, 0) == 0) return false;
    // 只让子进程继承写端, 否则本进程自己会一直占着管道, 永远等不到 EOF
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
    // 不要 stderr 时给它一个 NUL 句柄 (否则 PowerShell 的 CLIXML 会混进 stdout)
    HANDLE nul_stderr = INVALID_HANDLE_VALUE;
    if (merge_stderr == false) nul_stderr = CreateFileA("NUL", GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_EXISTING, 0, NULL);
    si.hStdError = (merge_stderr || nul_stderr == INVALID_HANDLE_VALUE) ? pipe_write : nul_stderr;
    si.hStdInput = (nul_input == INVALID_HANDLE_VALUE) ? NULL : nul_input;

    PROCESS_INFORMATION pi;
    memset(&pi, 0, sizeof(pi));

    const BOOL ok = CreateProcessW(w_exe, w_cmdline, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
    const DWORD create_error = ok ? 0 : GetLastError();

    if (nul_input != INVALID_HANDLE_VALUE) CloseHandle(nul_input);
    if (nul_stderr != INVALID_HANDLE_VALUE) CloseHandle(nul_stderr);
    CloseHandle(pipe_write); // 本进程不再需要写端

    if (ok == 0)
    {
        CloseHandle(pipe_read);
        LOG_WARN("启动子进程失败 (错误码 %lu)", (unsigned long)create_error);
        return false;
    }
    CloseHandle(pi.hThread);

    size_t used = 0;
    bool finished = false;
    const ULONGLONG deadline = GetTickCount64() + (ULONGLONG)timeout_ms;
    while (true)
    {
        child_drain(pipe_read, out, out_len, &used);

        const DWORD wait = WaitForSingleObject(pi.hProcess, CHILD_POLL_MS);
        if (wait == WAIT_OBJECT_0)
        {
            finished = true;
            break;
        }
        if (GetTickCount64() >= deadline) break;
    }

    if (finished)
    {
        // 子进程退出后管道里可能还有尾巴, 读干净再走
        for (int i = 0; i < 50; i++)
        {
            if (child_drain(pipe_read, out, out_len, &used) == false) break;
            Sleep(1);
        }
        DWORD code = 0;
        if (GetExitCodeProcess(pi.hProcess, &code)) { if (exit_code != NULL) *exit_code = (int)code; }
    }
    else
    {
        LOG_WARN("子进程超时未退出 (超过 %d 毫秒), 强制结束", timeout_ms);
        TerminateProcess(pi.hProcess, 1);
        WaitForSingleObject(pi.hProcess, 1000);
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pipe_read);
    out[used] = '\0';
    return finished;
}

#else

bool child_run_capture(const char* exe, const char* args, char* out, const size_t out_len,
                       const int timeout_ms, int* exit_code, const bool merge_stderr)
{
    (void)exe;
    (void)args;
    (void)timeout_ms;
    if (out != NULL && out_len > 0) out[0] = '\0';
    if (exit_code != NULL) *exit_code = -1;
    return false;
}

#endif
