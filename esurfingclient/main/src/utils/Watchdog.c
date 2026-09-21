#include "utils/Watchdog.h"

#include "states/States.h"

#include "utils/sim/SimThread.h"

#include "utils/PlatformUtils.h"
#include "utils/Logger.h"

#ifdef _WIN32
#include <stdlib.h>
#endif

/** @brief 看门狗线程的检查间隔 */
#define WATCHDOG_TICK_MS 500

/**
 * @brief 判定卡死的宽限
 *
 * 声明了预算之后再宽限这么久还没等到下一次打卡才动手。
 * 留余量是因为主循环的一轮本身会有零点几秒的抖动
 */
#define WATCHDOG_MARGIN_MS 5000

/**
 * @brief 打卡预算的下限
 *
 * 调用方传了个过小的值 (比如 0) 时兜一下, 免得把正常的一轮误判成卡死
 */
#define WATCHDOG_MIN_BUDGET_MS 1000

/** @brief 网络请求打卡在"连接超时 + 操作超时"之外再留的余量 */
#define WATCHDOG_NET_MARGIN_MS 10000

/**
 * @brief 启动后给主循环的启动宽限
 *
 * 线程刚起来时主循环可能还在做初始化, 还没打第一次卡
 */
#define WATCHDOG_START_GRACE_MS 30000

/**
 * @brief 线程间共享的状态
 *
 * 刻意只用 32 位量: OpenWrt 路由器多是 32 位 MIPS, 64 位量在两个线程之间
 * 读写会被撕裂, 可能算出错误的超时。
 *
 * - s_pet_seq: 打卡序号, 只增不减
 * - s_budget_ms: 本次打卡声明的预算
 *
 * 写入顺序是"先写预算, 再自增序号"。看门狗读到新序号时预算必然已是新的;
 * 万一只读到新序号却拿到旧预算, 那也只会让超时判得更早一点 —— 所以判断时
 * 先读序号、再读预算, 最后确认序号没变。
 */
static volatile uint32_t s_pet_seq = 0;
static volatile uint32_t s_budget_ms = 0;
static volatile bool s_running = false;
static sim_thread_t* s_thread = NULL;

/**
 * @brief 当前线程是不是看门狗自己
 *
 * 必须有这个标记: 本线程的循环也要睡眠, 而 sleep_ms() 现在会按睡眠时长打卡 ——
 * 那等于看门狗给自己打卡, 序号每个 tick 都在变, 判定分支永远走"有新打卡"这一边,
 * watchdog_fire 就成了不可达代码。表现是"装了看门狗但卡死依然没人管",
 * 而且不报任何错。
 *
 * 用线程局部变量而不是全局量: 只有本线程该被排除, 其它线程照常打卡。
 */
static _Thread_local bool tl_in_watchdog = false;

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
    uint64_t expected_by = get_cur_tm_ms() + budget_ms;
    uint64_t deadline = expected_by + WATCHDOG_MARGIN_MS;

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

        const uint32_t seq = s_pet_seq;
        if (first_tick || seq != last_seq)
        {
            budget_ms = s_budget_ms;
            last_seq = seq;
            expected_by = get_cur_tm_ms() + (uint64_t)budget_ms;
            deadline = expected_by + WATCHDOG_MARGIN_MS;
            first_tick = false;
            continue;
        }

        const uint64_t now = get_cur_tm_ms();
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
