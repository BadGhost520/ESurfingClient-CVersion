#include "TimeControl.h"
#include "States.h"
#include "utils/PlatformUtils.h"
#include "utils/Logger.h"
#include "utils/SimThread.h"

#include <stdint.h>
#include <time.h>

#define WEEK_MILLIS 604800000LL
#define MAX_SLEEP_SLICE_MS 10000

static sim_thread_t* g_time_control_thread = NULL;

/**
 * @brief 获取当前本地时间的“一周分钟数”(0-10079, 0=周日 00:00)
 */
static int32_t get_current_week_minute(void)
{
    time_t now = time(NULL);
    struct tm local_tm;
#ifdef _WIN32
    if (localtime_s(&local_tm, &now) != 0)
    {
        return 0;
    }
#else
    if (localtime_r(&now, &local_tm) == NULL)
    {
        return 0;
    }
#endif
    return (int32_t)local_tm.tm_wday * 1440 + local_tm.tm_hour * 60 + local_tm.tm_min;
}

/**
 * @brief 获取当前本地时间的“一周毫秒数”(0-604799999)
 */
static int64_t get_current_week_ms(void)
{
    time_t now = time(NULL);
    struct tm local_tm;
#ifdef _WIN32
    if (localtime_s(&local_tm, &now) != 0)
    {
        return 0;
    }
#else
    if (localtime_r(&now, &local_tm) == NULL)
    {
        return 0;
    }
#endif
    return ((int64_t)local_tm.tm_wday * 86400 +
            local_tm.tm_hour * 3600 +
            local_tm.tm_min * 60 +
            local_tm.tm_sec) * 1000;
}

/**
 * @brief 判断某个绝对周分钟是否落在任意时间窗口内
 *
 * 窗口按周循环，end_week_min 可能大于 WEEK_MINUTES（跨周）。
 * 通过 (t - start) mod WEEK_MINUTES 判断是否位于窗口持续时间内。
 */
static bool is_in_windows_abs(const login_cfg_t* cfg, const int32_t week_min_abs)
{
    for (uint8_t i = 0; i < cfg->time_window_count; i++)
    {
        const time_window_t* win = &cfg->time_windows[i];
        const int32_t start = win->start_week_min;
        const int32_t duration = (int32_t)win->end_week_min - start;

        int32_t rel = (week_min_abs - start) % WEEK_MINUTES;
        if (rel < 0) rel += WEEK_MINUTES;

        if (rel < duration)
        {
            return true;
        }
    }
    return false;
}

/**
 * @brief 计算下一个状态切换（启用/禁用）的延迟毫秒数
 * @return 延迟毫秒数，无窗口时返回 (uint64_t)-1
 */
static uint64_t compute_next_event_delay_ms(const login_cfg_t* cfg, const int64_t now_week_ms)
{
    int64_t boundaries[MAX_TIME_WINDOWS * 10];
    int boundary_count = 0;

    for (uint8_t i = 0; i < cfg->time_window_count; i++)
    {
        const time_window_t* win = &cfg->time_windows[i];
        const int64_t start_ms = (int64_t)win->start_week_min * 60000;
        const int64_t end_ms = (int64_t)win->end_week_min * 60000;

        // k 覆盖当前周前后足够多的周期，保证能找到下一个边界
        for (int k = -2; k <= 2; k++)
        {
            const int64_t start_abs = start_ms + k * WEEK_MILLIS;
            const int64_t end_abs = end_ms + k * WEEK_MILLIS;

            if (start_abs > now_week_ms && boundary_count < (int)(sizeof(boundaries) / sizeof(boundaries[0])))
            {
                boundaries[boundary_count++] = start_abs;
            }
            if (end_abs > now_week_ms && boundary_count < (int)(sizeof(boundaries) / sizeof(boundaries[0])))
            {
                boundaries[boundary_count++] = end_abs;
            }
        }
    }

    if (boundary_count == 0)
    {
        return (uint64_t)-1;
    }

    // 简单插入排序
    for (int i = 1; i < boundary_count; i++)
    {
        const int64_t key = boundaries[i];
        int j = i - 1;
        while (j >= 0 && boundaries[j] > key)
        {
            boundaries[j + 1] = boundaries[j];
            j--;
        }
        boundaries[j + 1] = key;
    }

    // 找到第一个让“是否在窗口内”发生变化的边界
    for (int i = 0; i < boundary_count; i++)
    {
        const int64_t t = boundaries[i];
        if (i > 0 && t == boundaries[i - 1])
        {
            continue;
        }

        const bool before = is_in_windows_abs(cfg, (int32_t)((t - 1) / 60000));
        const bool after = is_in_windows_abs(cfg, (int32_t)(t / 60000));
        if (before != after)
        {
            return (uint64_t)(t - now_week_ms);
        }
    }

    return (uint64_t)-1;
}

/**
 * @brief 按当前时间重新同步所有账号的 time_windows 启用/禁用状态
 *
 * 设计说明：
 * - 冷启动时由 work() 调用一次，之后定时线程每次醒来都会调用；
 * - 这样即使设备休眠/系统时间跳变，醒来后也会按“当前时间”重新校正状态，
 *   而不是机械执行睡前的旧事件。
 *
 * 线程安全：
 * - 这里沿用项目现有的跨线程裸 bool 风格（is_running/is_need_reset 等同样如此）。
 * - 严格场景下应改为 C11 原子操作或加锁，但为了与现有代码保持一致暂不引入。
 */
void time_control_sync(void)
{
    if (g_prog_status == NULL || g_prog_cnt <= 0)
    {
        return;
    }

    const int32_t now_week_min = get_current_week_minute();

    for (uint8_t i = 0; i < g_prog_cnt; i++)
    {
        login_cfg_t* cfg = &g_prog_status[i].login_cfg;
        if (cfg->has_time_control == false)
        {
            // 没有时间控制的账号保持默认启用；防止保存配置去掉 time_windows 后残留禁用状态
            if (g_prog_status[i].runtime_status.is_time_disabled)
            {
                g_prog_status[i].runtime_status.is_time_disabled = false;
                g_prog_status[i].runtime_status.is_need_reset = false;
                LOG_INFO("配置 %" PRIu8 " 已取消时间控制，恢复默认启用", cfg->idx);
            }
            continue;
        }

        const bool in_window = is_in_windows_abs(cfg, now_week_min);
        const bool was_disabled = g_prog_status[i].runtime_status.is_time_disabled;

        if (in_window && was_disabled)
        {
            g_prog_status[i].runtime_status.is_time_disabled = false;
            g_prog_status[i].runtime_status.is_need_reset = false;
            LOG_INFO("配置 %" PRIu8 " 已进入允许时段，等待线程守护启动", cfg->idx);
        }
        else if (in_window == false)
        {
            // 只要不在允许时段就持续请求下线，防止线程内部 reset/clean 清掉 is_need_reset 后继续运行
            g_prog_status[i].runtime_status.is_time_disabled = true;
            g_prog_status[i].runtime_status.is_need_reset = true;
            if (was_disabled == false)
            {
                LOG_INFO("配置 %" PRIu8 " 已离开允许时段，请求下线", cfg->idx);
            }
        }
    }
}

/**
 * @brief 时间控制定时线程主循环
 *
 * 每次醒来先按当前时间校正状态，再计算所有账号中“最近的下一次切换”并睡眠。
 */
static int time_control_app(void* arg)
{
    (void)arg;
    tl_thread_idx = -1;

    LOG_INFO("时间控制线程已启动");

    if (g_prog_cnt <= 0)
    {
        LOG_INFO("没有可用账号，时间控制线程退出");
        return 0;
    }

    while (g_thread_keep_alive && g_need_exit == false)
    {
        time_control_sync();

        const int64_t now_week_ms = get_current_week_ms();
        uint64_t next_delay = (uint64_t)-1;

        for (uint8_t i = 0; i < g_prog_cnt; i++)
        {
            const login_cfg_t* cfg = &g_prog_status[i].login_cfg;
            if (cfg->has_time_control == false)
            {
                continue;
            }

            const uint64_t delay = compute_next_event_delay_ms(cfg, now_week_ms);
            if (delay != (uint64_t)-1 && delay < next_delay)
            {
                next_delay = delay;
            }
        }

        if (next_delay == (uint64_t)-1)
        {
            // 没有时间控制账号，短暂睡眠后继续检查，便于保存配置后能较快感知变化
            sleep_ms(1000, true);
            continue;
        }

        if (next_delay == 0)
        {
            // 避免极端情况下忙等
            next_delay = 1;
        }

        // 精确睡到下一个边界，但最多只睡 MAX_SLEEP_SLICE_MS 就重新校正一次：
        // 正常情况最后一段会精确落在边界上；休眠/时间跳变时也能在切片时间内纠正。
        const uint64_t slice = next_delay > MAX_SLEEP_SLICE_MS ? MAX_SLEEP_SLICE_MS : next_delay;
        sleep_ms(slice, true);
    }

    LOG_INFO("时间控制线程已退出");
    return 0;
}

bool time_control_init(void)
{
    if (g_time_control_thread != NULL)
    {
        return true;
    }

    if (g_prog_status == NULL || g_prog_cnt <= 0)
    {
        return true;
    }

    bool has_time_control = false;
    for (uint8_t i = 0; i < g_prog_cnt; i++)
    {
        if (g_prog_status[i].login_cfg.has_time_control)
        {
            has_time_control = true;
            break;
        }
    }

    // 没有任何账号启用时间控制时不需要创建定时线程
    if (has_time_control == false)
    {
        return true;
    }

    uint8_t retry = 1;
    g_time_control_thread = sim_thread_create(time_control_app, NULL);
    while (g_time_control_thread == NULL)
    {
        if (retry > 5)
        {
            LOG_FATAL("时间控制线程创建失败");
            return false;
        }
        LOG_ERROR("时间控制线程创建失败, 重试中, 重试次数: %" PRIu8 ", 最多 5 次", retry);
        g_time_control_thread = sim_thread_create(time_control_app, NULL);
        retry++;
    }

    return true;
}

void time_control_stop(void)
{
    if (g_time_control_thread == NULL)
    {
        return;
    }

    int result_code = 0;
    sim_thread_join(g_time_control_thread, &result_code);
    g_time_control_thread = NULL;
    LOG_DEBUG("时间控制线程退出, 退出码: %d", result_code);
}
