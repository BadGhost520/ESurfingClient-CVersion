#ifndef ESURFINGCLIENT_CHILDRUN_H
#define ESURFINGCLIENT_CHILDRUN_H

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief 跑一个子进程, 收走它的 stdout/stderr (一次性, 跑完就退)
 *
 * 只给"必须借外部程序才办得到"的事用: Windows 的热点开关只有 WinRT 接口,
 * 而 C11 没有 WinRT 投影, 只能让 Windows PowerShell 跑一次拿结果。
 * 子进程不开窗口, stdout 与 stderr 合并成一份输出。
 *
 * 超时会强杀子进程并返回 false —— 不能让一次热点操作把 Web 线程永远卡住。
 * 注意这是阻塞调用, 会占住调用线程, 不要在 Web 请求线程里直接跑耗时操作。
 *
 * @param exe 可执行文件绝对路径 (UTF-8)
 * @param args 命令行参数 (UTF-8; 含空格的路径要自己加好引号)
 * @param out 收输出的缓冲 (总以 NUL 结尾)
 * @param out_len 缓冲长度
 * @param timeout_ms 最长等多久
 * @param exit_code 子进程退出码 (可为 NULL)
 * @param merge_stderr 是否把 stderr 也收进 out (PowerShell 会把进度/错误以 CLIXML
 *                     写到 stderr, 只要解析它 stdout 上的 JSON 时应当传 false)
 * @return 是否跑完 (启动失败或超时返回 false)
 */
bool child_run_capture(const char* exe, const char* args, char* out, const size_t out_len,
                       const int timeout_ms, int* exit_code, const bool merge_stderr);

#endif
