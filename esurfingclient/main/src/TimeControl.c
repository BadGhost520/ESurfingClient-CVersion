#include "TimeControl.h"
#include "States.h"
#include "utils/PlatformUtils.h"
#include "utils/Logger.h"
#include "utils/SimThread.h"

#include <stdint.h>
#include <time.h>

static sim_thread_t* g_time_control_thread = NULL;

/**
 * @brief 获取当前本地时间的“分钟数”(0-1439)
 */
static uint16_t get_current_minute(void)
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
    return (uint16_t)(local_tm.tm_hour * 60 + local_tm.tm_min);
}

/**
 * @brief 计算从 now 到下一个 target_min 时刻的毫秒数
 * @note 如果目标时刻已经过去，则返回明天同一时刻的延迟
 */
static uint64_t compute_delay_to_minute(const uint16_t target_min, const time_t now)
{
    struct tm target_tm;
#ifdef _WIN32
    if (localtime_s(&target_tm, &now) != 0)
    {
        return (uint64_t)-1;
    }
#else
    if (localtime_r(&now, &target_tm) == NULL)
    {
        return (uint64_t)-1;
    }
#endif

    target_tm.tm_hour = target_min / 60;
    target_tm.tm_min = target_min % 60;
    target_tm.tm_sec = 0;
    target_tm.tm_isdst = -1;

    time_t target = mktime(&target_tm);
    if (target == (time_t)-1)
    {
        return (uint64_t)-1;
    }

    if (target <= now)
    {
        target_tm.tm_mday += 1;
        target = mktime(&target_tm);
        if (target == (time_t)-1)
        {
            return (uint64_t)-1;
        }
    }

    return (uint64_t)(target - now) * 1000;
}

/**
 * @brief 按当前时间重新同步所有账号的 time_range 启用/禁用状态
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

    const uint16_t now_min = get_current_minute();

    for (uint8_t i = 0; i < g_prog_cnt; i++)
    {
        login_cfg_t* cfg = &g_prog_status[i].login_cfg;
        if (cfg->has_time_control == false)
        {
            // 没有时间控制的账号保持默认启用；防止保存配置去掉 time_range 后残留禁用状态
            if (g_prog_status[i].runtime_status.is_time_disabled)
            {
                g_prog_status[i].runtime_status.is_time_disabled = false;
                g_prog_status[i].runtime_status.is_need_reset = false;
                LOG_INFO("配置 %" PRIu8 " 已取消时间控制，恢复默认启用", cfg->idx);
            }
            continue;
        }

        const bool in_window = (now_min >= cfg->time_start_min && now_min < cfg->time_end_min);
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

        const uint16_t now_min = get_current_minute();
        const time_t now = time(NULL);
        uint64_t next_delay = (uint64_t)-1;

        for (uint8_t i = 0; i < g_prog_cnt; i++)
        {
            const login_cfg_t* cfg = &g_prog_status[i].login_cfg;
            if (cfg->has_time_control == false)
            {
                continue;
            }

            const bool in_window = (now_min >= cfg->time_start_min && now_min < cfg->time_end_min);
            const uint16_t target_min = in_window ? cfg->time_end_min : cfg->time_start_min;
            const uint64_t delay = compute_delay_to_minute(target_min, now);

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
            next_delay = 1000;
        }

        // 最多睡 1 秒就回到循环重新校正一次：
        // 这样设备休眠/系统时间跳变后，最多 1 秒内就会按“当前时间”重新同步状态。
        sleep_ms(next_delay > 1000 ? 1000 : next_delay, true);
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
