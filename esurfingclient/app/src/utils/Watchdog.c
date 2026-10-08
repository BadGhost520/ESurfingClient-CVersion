#include "utils/Watchdog.h"

#include "states/States.h"

#include "utils/sim/SimThread.h"

#include "utils/PlatformUtils.h"
#include "utils/Logger.h"

#ifdef _WIN32
#include <stdlib.h>
#endif

#define WATCHDOG_TICK_MS 500

#define WATCHDOG_MARGIN_MS 5000

#define WATCHDOG_MIN_BUDGET_MS 1000

#define WATCHDOG_NET_MARGIN_MS 10000

#define WATCHDOG_START_GRACE_MS 30000

/* 墙钟与单调钟的进度差超过这个值, 就认定是墙钟被调整, 不是主循环卡住 */
#define WATCHDOG_CLOCK_JUMP_MS 3000

#define WATCHDOG_MS_PER_DAY 86400000ULL

#define WATCHDOG_MS_PER_HOUR 3600000ULL

#define WATCHDOG_MS_PER_MIN 60000ULL

static volatile uint32_t s_pet_seq = 0;
static volatile uint32_t s_budget_ms = 0;
static volatile bool s_running = false;
static sim_thread_t* s_thread = NULL;

static _Thread_local bool tl_in_watchdog = false;

/* 把毫秒折成可读时长: 墙钟一跳几十小时时, 原始毫秒数根本看不出量级 */
static void format_duration(const uint64_t ms, char* buf, const size_t buf_size)
{
    const uint64_t days = ms / WATCHDOG_MS_PER_DAY;
    const uint64_t hours = ms % WATCHDOG_MS_PER_DAY / WATCHDOG_MS_PER_HOUR;
    const uint64_t mins = ms % WATCHDOG_MS_PER_HOUR / WATCHDOG_MS_PER_MIN;
    const uint64_t secs = ms % WATCHDOG_MS_PER_MIN / 1000;
    const uint64_t millis = ms % 1000;

    if (days > 0) snprintf(buf, buf_size, "%" PRIu64 " 天 %" PRIu64 " 小时", days, hours);
    else if (hours > 0) snprintf(buf, buf_size, "%" PRIu64 " 小时 %" PRIu64 " 分", hours, mins);
    else if (mins > 0) snprintf(buf, buf_size, "%" PRIu64 " 分 %" PRIu64 " 秒", mins, secs);
    else snprintf(buf, buf_size, "%" PRIu64 ".%03" PRIu64 " 秒", secs, millis);
}

/**
 * @brief 判定卡死后的动作
 *
 * 不能调 shut(): 它会 join 各个线程, 可能正好卡在同一个死锁上, 那就一起卡住了。
 * 也不能指望常规的 LOG_* —— 日志虽然是无锁的, 但这里要尽量少做事。
 * 所以只做一次直达 fd 的 write, 然后立刻退出, 交给外部监管者重新拉起。
 * @param budget_ms 超时时声明的预算
 * @param overdue_ms 超出预算多久
 */
static void watchdog_fire(const uint32_t budget_ms, const uint64_t overdue_ms)
{
    char msg[256];
    snprintf(msg, sizeof(msg),
        WATCHDOG_KILL_MARK ": 主循环声明 %u 毫秒内会再来打卡, 已超时 %llu 毫秒没有动静. "
        "本进程将退出, 由外部监管者 (procd / systemd / SCM) 重新拉起",
        budget_ms, (unsigned long long)overdue_ms);

    log_raw_line(msg);

    /**
     * 用 _exit 而不是 exit: exit 会跑 atexit 回调、刷 stdio 缓冲, 那些都可能
     * 卡在同一个问题上。这里要的是"立刻死掉让监管者重启"
     */
    _exit(1);
}

/**
 * @brief 看门狗线程主循环
 * @param arg 未使用
 * @return 恒为 0 (判定卡死时走 _exit, 不会返回)
 */
static int watchdog_app(void* arg)
{
    (void)arg;
    tl_in_watchdog = true; // 本线程的睡眠不算"主循环在动", 见 tl_in_watchdog 的说明
    tl_thread_idx = -1;
    tl_thread_name = "watchdog";

    uint32_t last_seq = s_pet_seq;
    uint32_t budget_ms = s_budget_ms;

    /**
     * 计时必须用单调钟, 不能用 get_cur_tm_ms() 的墙钟。
     *
     * OpenWrt 没有 RTC, 开机后一连上网 NTP 就把墙钟整体前跳 (跳几十小时很常见),
     * 而主循环此时正安静地睡着 —— 墙钟一减就凭空多出"已经过了 47 小时",
     * 于是一次正常的心跳等待被当场判成卡死并 _exit。实测日志里那次
     * "声明 1000 毫秒, 已超时 171888236 毫秒"就是这么来的。
     * 单调钟只受"真的过了多久"影响, 改时钟它不动。
     */
    uint64_t expected_by = get_steady_tm_ms() + budget_ms;
    uint64_t deadline = expected_by + WATCHDOG_MARGIN_MS;
    uint64_t last_steady = get_steady_tm_ms();
    uint64_t last_wall = get_cur_tm_ms();

    /**
     * 第一轮必定重新取一次预算。
     *
     * 少了这一步会有个竞态: 主循环可能在【本线程读到初始序号之前】就已经打过卡了,
     * 那次打卡会被当成"序号没变"而丢掉, 预算于是永远是初始值 —— 表现就是
     * 明明有打卡, 判定时却报"声明 0 毫秒", 而且按启动宽限而不是按声明的预算计时。
     * (这不是想出来的, 是测试日志里那句"声明 0 毫秒"暴露出来的)
     */
    bool first_tick = true;

    while (s_running)
    {
        sleep_ms(WATCHDOG_TICK_MS, true);

        /**
         * 关闭流程中主循环本来就不再打卡了, 那不是卡死。
         * 这里必须放行, 否则正常退出会被误判, 还会盖掉真正的退出原因
         */
        if (g_need_exit || g_stop_requested) return 0;

        // 打卡分支也要走单调钟: 这里算出来的 expected_by 直接参与后面的判定
        const uint64_t pet_now = get_steady_tm_ms();

        const uint32_t seq = s_pet_seq;
        if (first_tick || seq != last_seq)
        {
            budget_ms = s_budget_ms;
            last_seq = seq;
            expected_by = pet_now + (uint64_t)budget_ms;
            deadline = expected_by + WATCHDOG_MARGIN_MS;
            first_tick = false;
            last_steady = pet_now;
            last_wall = get_cur_tm_ms();
            continue;
        }

        const uint64_t now = pet_now;
        const uint64_t step = now - last_steady;
        const uint64_t wall_now = get_cur_tm_ms();
        const uint64_t wall_step = wall_now - last_wall;

        /**
         * 判定开枪之前先看两个钟对不对得上。
         *
         * 判据是【两个钟的进度互相矛盾】(比如单调钟只走了几毫秒, 墙钟却跳了 47 小时),
         * 而不是"超过了某个绝对时长" —— 后者会把真卡死一起放过。
         * 真卡死时两个钟是一起走的, 这里不会成立, 该开枪还是开枪。
         * 顺带把墙钟为什么对不上记进日志, 否则下次还得靠人工拿两条时间戳相减去猜。
         */
        if (step + WATCHDOG_CLOCK_JUMP_MS < wall_step || wall_step + WATCHDOG_CLOCK_JUMP_MS < step)
        {
            char moved[64], elapsed[64];
            format_duration(wall_step > step ? wall_step - step : step - wall_step, moved, sizeof(moved));
            format_duration(step, elapsed, sizeof(elapsed));
            LOG_WARN("检测到墙钟被调整 (跳变约 %s, 单调钟同期只走了 %s), 已重新计时; "
                "NTP 校时不算主循环卡死", moved, elapsed);
        }

        last_steady = now;
        last_wall = wall_now;

        if (now > deadline)
        {
            /**
             * 动手前再确认一次。
             *
             * 从上面那次检查到现在, 另一线程可能已经进了 shut() —— 它会
             * watchdog_stop() 把 s_running 置假, 但【取消不了已经越过检查的这一次】。
             * 不复查的话, 一次正常关闭会被写成"看门狗判定卡死", 而且跳过 clean_logger()
             * (日志就不改名了)。
             */
            if (s_running == false || g_need_exit || g_stop_requested) return 0;

            watchdog_fire(budget_ms, now - expected_by);
        }
    }

    return 0;
}

bool watchdog_start(void)
{
    if (s_running) return true;

    /**
     * 初始预算取启动宽限而不是 0: 主循环还没打第一次卡之前, 按这个等。
     * 取 0 的话第一轮就会按"声明 0 毫秒"计时, 立刻误判
     */
    s_budget_ms = WATCHDOG_START_GRACE_MS;
    s_pet_seq = 0;
    s_running = true;

    s_thread = sim_thread_create(watchdog_app, NULL);
    if (s_thread == NULL)
    {
        s_running = false;
        LOG_ERROR("看门狗线程创建失败, 本进程将无法发现自己卡死");
        return false;
    }

    LOG_INFO("看门狗已启动 (检查间隔 %d 毫秒, 宽限 %d 毫秒)", WATCHDOG_TICK_MS, WATCHDOG_MARGIN_MS);
    return true;
}

void watchdog_stop(void)
{
    /**
     * ⚠️ 本函数【不是】线程安全的: 先判 s_running 再置假, 两个线程同时进来
     *    会各自去 join 同一条线程。
     *
     * 目前不会发生, 因为:
     *   - 看门狗只在认证进程里启动, 而那边 dialer_app 与 shut() 都跑在主线程上, 是顺序执行的
     *   - shut() 本身有 shutting_down 把关, 关闭流程只会被一个线程走完
     * 但将来若出现"从 SCM 线程调 shut() 且该进程启动了看门狗"的组合, 就会踩到。
     * 到那时要给这里补一把真正的锁 —— 用 volatile 标志假冒是不行的。
     */
    if (s_running == false) return;

    s_running = false;

    if (s_thread != NULL)
    {
        int result_code = 0;
        sim_thread_join(s_thread, &result_code);
        s_thread = NULL;
    }
}

void watchdog_pet(uint32_t budget_ms)
{
    if (s_running == false) return;

    // 看门狗自己不算数 (它一打卡, 判定就永远走"有新打卡"这一边)
    if (tl_in_watchdog) return;

    if (budget_ms < WATCHDOG_MIN_BUDGET_MS) budget_ms = WATCHDOG_MIN_BUDGET_MS;

    // 先写预算再自增序号, 顺序不能反 (见 s_pet_seq 的说明)
    s_budget_ms = budget_ms;
    s_pet_seq++;
}

void watchdog_pet_network(void)
{
    if (s_running == false) return;
    if (tl_in_watchdog) return;

    /**
     * 一次网络请求最长就是连接超时 + 操作超时, 再加一点余量。
     * 逐个请求地覆盖, 而不是估"一轮循环最多几次请求" —— 后者只要估小一次
     * 就会把正常的慢请求误判成卡死 (而误杀比漏报严重得多)。
     */
    const uint64_t budget = (uint64_t)(g_conn_timeout + g_op_timeout) * 1000ULL + WATCHDOG_NET_MARGIN_MS;

    watchdog_pet((uint32_t)(budget > UINT32_MAX ? UINT32_MAX : budget));
}
