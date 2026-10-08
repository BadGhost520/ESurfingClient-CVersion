#include "utils/PlatformUtils.h"

#include "states/States.h"

#include "utils/PlatformInternal.h"
#include "utils/Watchdog.h"

#include <stdio.h>
#include <time.h>

uint64_t get_cur_tm_ms()
{
#ifdef _WIN32
    FILETIME ft;
    ULARGE_INTEGER uli;
    GetSystemTimeAsFileTime(&ft);
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    return uli.QuadPart / 10000LL - 11644473600000LL;
#else
    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0) return 0;
    return tv.tv_sec * 1000LL + tv.tv_usec / 1000LL;
#endif
}

uint64_t get_steady_tm_ms()
{
#ifdef _WIN32
    /**
     * QueryPerformanceCounter: 单调递增, 不受系统时间调整影响。
     * 频率由 QueryPerformanceFrequency 给出, 不一定是 1000, 所以要换算,
     * 不能像 get_cur_tm_ms() 那样直接把计数当毫秒用
     */
    LARGE_INTEGER freq, counter;
    if (QueryPerformanceFrequency(&freq) == 0 || freq.QuadPart == 0) return 0;
    QueryPerformanceCounter(&counter);
    return (uint64_t)(counter.QuadPart / freq.QuadPart * 1000
        + counter.QuadPart % freq.QuadPart * 1000 / freq.QuadPart);
#else
    /**
     * CLOCK_MONOTONIC: 从启动起单调递增, NTP / settimeofday 改墙钟时它不动。
     * 这里不能用 gettimeofday(): OpenWrt 开机后 NTP 校时会把墙钟整体前跳,
     * 那一下会被当成"已经过了很久"
     */
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0;
    return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)ts.tv_nsec / 1000000ULL;
#endif
}

void sleep_ms(const uint64_t ms, const bool can_stop)
{
    if (ms == 0) return;

    /**
     * 睡眠是最常见的"长时间没动静", 但它【不是卡死】—— 它本身就是在声明
     * "我会安静这么久"。所以在这里打卡, 预算就是这次睡眠的时长。
     *
     * 这一处不能省: 认证失败后的退避会睡 60 秒到 30 分钟 (见 DialerClient.c 的
     * table[]), 不打卡的话看门狗会把它当成卡死, 于是"认证失败 -> 退避 ->
     * 被杀 -> 重启 -> 再失败"变成杀循环。
     *
     * 注意它与网络请求处的打卡是配套的: 这里可能把预算改小 (睡 1 秒),
     * 而紧接着的网络请求会自己再打一次卡把预算调回来, 所以不会误杀。
     */
    watchdog_pet(ms > UINT32_MAX ? UINT32_MAX : (uint32_t)ms);

    if (can_stop)
    {
        uint64_t elapsed = 0;

        while (elapsed < ms && g_thread_keep_alive && g_stop_requested == 0)
        {
            if (tl_thread_idx > -1)
            {
                if (g_prog_status[tl_thread_idx].runtime_status.is_running == false || g_prog_status[tl_thread_idx].runtime_status.is_need_reauth)
                {
                    return;
                }
            }
            else
            {
                if (g_need_exit)
                {
                    return;
                }
            }
            const uint64_t SEGMENT_MS = 100;
            const uint64_t sleep_time = ms - elapsed < SEGMENT_MS ? ms - elapsed : SEGMENT_MS;

#ifdef _WIN32
            Sleep(sleep_time);
#else
            usleep(sleep_time * 1000);
#endif
            elapsed += sleep_time;
        }
    }
    else
    {
#ifdef _WIN32
        Sleep(ms);
#else
        usleep(ms * 1000);
#endif
    }
}

void get_fmt_time(char* buf, const TimeFormat fmt)
{
    time_t raw_tm;
    if (time(&raw_tm) == (time_t) - 1)
    {
        fprintf(stderr, "ERROR: 获取系统时间失败\n");
        return;
    }
    struct tm local_tm;
#ifdef _WIN32
    if (localtime_s(&local_tm, &raw_tm) != 0)
    {
        fprintf(stderr, "ERROR: 时间转换失败\n");
        return;
    }
#else
    if (localtime_r(&raw_tm, &local_tm) == NULL)
    {
        fprintf(stderr, "ERROR: 时间转换失败\n");
        return;
    }
#endif
    switch (fmt)
    {
    case CONSOLE_FORMAT:
        if (strftime(buf, 32, "%Y-%m-%d %H:%M:%S", &local_tm) == 0)
        {
            fprintf(stderr, "ERROR: 格式化时间失败\n");
            return;
        }
        return;
    case FILE_FORMAT:
        if (strftime(buf, 32, "%Y%m%d-%H%M%S", &local_tm) == 0)
        {
            fprintf(stderr, "ERROR: 格式化时间失败\n");
        }
    }
}
