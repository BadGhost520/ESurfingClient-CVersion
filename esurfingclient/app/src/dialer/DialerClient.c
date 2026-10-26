#include "dialer/DialerClient.h"

#include "DialerInternal.h"

#include "config/Config.h"

#include "net/NetClient.h"

#include "dialer/ErrorCode.h"

#include "core/States.h"

#include "utils/time/TimeUtils.h"
#include "utils/time/TimeControl.h"
#include "utils/runtime/Shutdown.h"
#include "utils/runtime/Watchdog.h"
#include "utils/log/Logger.h"
#include "utils/sim/SimThread.h"

#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
extern bool get_service_mode(void);
#else
#include <fcntl.h>
#include <stdlib.h>
#include <sys/file.h>
#include <unistd.h>
#endif

#ifndef __OPENWRT__
#include "net/NetTraffic.h"

#include "supervisor/Supervisor.h"

#include "webserver/HotspotControl.h"

extern bool start_web_server_remote(void);
#endif

typedef enum
{
    RUN_SUCCESS = 0,
    RUN_FAILED = 1,
    TIMEOUT_RETRY = 2
} RunStatus;

/**
 * 认证失败后的退避间隔 (分钟)
 *
 * 网络/服务端临时故障按这张表走, 用完之后固定用最后一档, 一直重试;
 * 需要人工处理的错误不走这张表, 直接等到重启或退出。
 *
 * 最后一档特意取 35 而不是 30: 服务端对反复登录的惩罚是锁 30 分钟,
 * 卡着 30 分钟整点重试很可能还在锁定窗口里, 多等 5 分钟才稳。
 */
static const uint64_t table[] = {1, 3, 5, 10, 20, 35};

/**
 * @brief 各角色入口的公共起步动作
 *
 * 取全局状态、装关闭钩子、起日志、打横幅。认证进程 / Web 进程 / 监管进程
 * 三处原本各抄一份, 已经出现过细节漂移 (监管进程自己手写了横幅)。
 *
 * 横幅只由入口角色打: 监管进程 (桌面) 或单实例模式 (OpenWrt)。认证 / Web 子进程
 * 不打 —— 它们是程序的一部分, 每次被重新拉起都打一遍的话, 同一份 run.log 里会
 * 堆出好几份一模一样的版本信息, 控制台上也会刷三遍。
 * @return 日志系统是否初始化成功
 */
bool app_bootstrap(void)
{
    g_thread_keep_alive = true;

    if (g_prog_status == NULL)
    {
        g_prog_status = calloc(1, sizeof(prog_status_t));
        if (g_prog_status == NULL) return false;
    }

    init_shutdown_hook();

    if (init_logger() == false) return false;

    /**
     * 横幅只由"入口"打
     *
     * - 桌面上入口是监管进程: auth / web 子进程不打, 否则同一份 run.log 里
     *   会有三份一样的版本信息 (而且它们会反复重启, 越堆越多)
     * - OpenWrt 上没有监管进程, 是 init 脚本直接拉起 auth 实例 —— 所以那边由
     *   auth 进程打; 多账号时只让 --account 1 打, 免得一个账号一份
     * - 手动不带参数运行 (standalone) 也是入口, 照打
     */
#ifdef __OPENWRT__
    const bool need_banner = (g_prog_role != ROLE_WEB) && (g_prog_account <= 1);
#else
    const bool need_banner = (g_prog_role != ROLE_AUTH && g_prog_role != ROLE_WEB);
#endif

    if (need_banner)
    {
        print_banner();
    }

    return true;
}

/**
 * @brief 认证失败后等待, 期间可被重启 / 退出 / 停止打断
 * @param wait_minutes 等多少分钟; 0 表示一直等到重启或退出
 * @return true 正常等完 (可继续重试), false 被打断
 */
static bool wait_auth_retry(const uint64_t wait_minutes)
{
    const uint64_t segment_ms = 5000; // 每 5 秒复核一次标志
    const uint64_t total_ms = wait_minutes * 60000ULL;
    uint64_t waited_ms = 0;

    while (true)
    {
        if (g_thread_keep_alive == false || g_stop_requested != 0) return false;
        if (g_prog_status[tl_thread_idx].runtime_status.is_running == false) return false;
        if (g_prog_status[tl_thread_idx].runtime_status.is_need_reauth) return false;
        if (wait_minutes != 0 && waited_ms >= total_ms) return true;

        uint64_t step = segment_ms;
        if (wait_minutes != 0 && total_ms - waited_ms < step) step = total_ms - waited_ms;
        sleep_ms(step, true);
        waited_ms += step;
    }
}

static RunStatus run(void)
{
    static uint8_t retry_timeout = 1;
    static uint8_t retry_auth = 1;

    /** 连续 302 的确认计数: 要连续两次才认定需要重新认证 */
    static _Thread_local uint8_t s_need_auth_hits = 0;

    /** 最近两次登录的时间 (单调钟), 用来限流; 60 秒内满 2 次就停手等 35 分钟 */
    static _Thread_local uint64_t s_login_ms[2] = {0, 0};
    static _Thread_local uint8_t s_login_slot = 0;

    // 时间控制/重置请求优先于一切网络操作:
    // 到点下线后不应再发送心跳包，也不应继续认证或重试
    if (g_prog_status[tl_thread_idx].runtime_status.is_time_disabled)
    {
        g_prog_status[tl_thread_idx].runtime_status.is_need_reauth = true;
    }
    if (g_prog_status[tl_thread_idx].runtime_status.is_need_reauth)
    {
        return RUN_SUCCESS;
    }

    const network_status_t net_state = check_network_status(true); // 检测网络状态

    // 状态真的变了才提示 (已联网 / 需要认证), 免得每秒刷一条
    log_network_state_change(net_state);

    switch (net_state)
    {
    case STATUS_OK: // 正常联网
        retry_timeout = 1;
        retry_auth = 1;
        s_need_auth_hits = 0; // 中间有一次正常联网, 之前的 302 就不算"连续"了
        /**
         * 检测是否初始化会话和认证登录
         * 如果已经初始化会话和认证登录, 则进入, 否则按已连接互联网处理
         */
        if (g_prog_status[tl_thread_idx].runtime_status.is_initialized && g_prog_status[tl_thread_idx].runtime_status.is_authed)
        {
            if (g_prog_status[tl_thread_idx].auth_cfg.keep_retry != 0) // 检测重试时间是否为零
            {
                /**
                 * 检测经过的时间是否达到重试时间
                 * 达到就发送心跳包
                 *
                 * 用单调钟: 墙钟被 NTP 校时跳一下, 这个减法就会凭空多出几十小时,
                 * 于是刚发完心跳又立刻再发一次 (或反过来把心跳拖后很久)
                 */
                if (get_steady_tm_ms() - g_prog_status[tl_thread_idx].auth_cfg.tick >= g_prog_status[tl_thread_idx].auth_cfg.keep_retry * 1000)
                {
                    /**
                     * 心跳失败按递增间隔重试: 1 / 2 / 4 / 8 / 16 / 30 秒, 总窗口约 61 秒
                     *
                     * 失败原因分开处理:
                     * - 网络类 (连不上/超时): 重认证没意义 —— 窗口用完后不退出进程,
                     *   只把 tick 推到下个周期, 等网络恢复再试
                     * - 协议类 (服务端有响应但内容不对): 错误码是 FATAL/SLOW (密码错/被踢/
                     *   欠费) 时立即重新认证; 其余同样退避重试
                     *
                     * sleep_ms(..., true) 在退避期间仍可被重启/退出/重认证请求立刻打断。
                     */
                    static const uint32_t heartbeat_backoff_ms[] = {1000, 2000, 4000, 8000, 16000, 30000};
                    const uint8_t heartbeat_retry_max =
                        sizeof(heartbeat_backoff_ms) / sizeof(heartbeat_backoff_ms[0]);

                    uint8_t retry_heartbeat = 1;
                    hb_result_t hb = heartbeat();
                    while (hb != HB_OK)
                    {
                        if (hb == HB_PROTO_FAIL)
                        {
                            const auth_error_kind_t kind =
                                (auth_error_kind_t)g_prog_status[tl_thread_idx].runtime_status.last_error_kind;

                            if (kind == AUTH_ERROR_FATAL || kind == AUTH_ERROR_SLOW)
                            {
                                LOG_WARN("心跳被服务端拒绝 (分类 %d), 立即重新认证", (int)kind);
                                g_prog_status[tl_thread_idx].runtime_status.is_need_reauth = true;
                                return RUN_SUCCESS;
                            }
                        }

                        if (retry_heartbeat > heartbeat_retry_max)
                        {
                            if (hb == HB_NET_FAIL)
                            {
                                // 网络一直不通: 退登也没用, 下个心跳周期再试
                                LOG_ERROR("心跳包连续 %" PRIu8 " 次网络失败, 下个周期再试", heartbeat_retry_max);
                                g_prog_status[tl_thread_idx].auth_cfg.tick = get_steady_tm_ms();
                                return RUN_SUCCESS;
                            }

                            LOG_ERROR("心跳包连续 %" PRIu8 " 次失败, 放弃本轮", heartbeat_retry_max);
                            return RUN_FAILED;
                        }

                        const uint32_t wait_ms = heartbeat_backoff_ms[retry_heartbeat - 1];
                        LOG_WARN("配置 %" PRIu8 " 心跳包发送失败 (%s), 下标 %" PRIu8 ", 第 %" PRIu8 "/%" PRIu8 " 次, %" PRIu32 " 秒后重试",
                            g_prog_status[tl_thread_idx].login_cfg.idx,
                            hb == HB_NET_FAIL ? "网络不通" : "服务端拒绝",
                            tl_thread_idx,
                            retry_heartbeat,
                            heartbeat_retry_max,
                            wait_ms / 1000);

                        retry_heartbeat++;
                        sleep_ms(wait_ms, true);
                        hb = heartbeat();
                    }
                    LOG_DEBUG("下一次发送心跳包: %" PRIu64 " 秒后",
                        g_prog_status[tl_thread_idx].auth_cfg.keep_retry);
                    g_prog_status[tl_thread_idx].auth_cfg.tick = get_steady_tm_ms(); // 重新给 tick 赋值 (与上面同源, 都用单调钟)
                }
            }
        }
        // 未认证时不再重复打日志: log_network_state_change 已在状态变化时提示过一次
        sleep_ms(1000, false);
        return RUN_SUCCESS;
    case STATUS_NEED_AUTH: // 需要认证
        retry_timeout = 1;

        /**
         * 连续两次 302 才认定需要重新认证
         *
         * 单次 302 可能只是网关抖动、探测超时被误判。一次误判就会把好会话登出
         * 再重认证 —— 而重认证就是一次登录, 频繁登录会被服务端锁 30 分钟。
         */
        if (s_need_auth_hits < 2)
        {
            s_need_auth_hits++;
            LOG_DEBUG("检测到 302, 第 %" PRIu8 "/2 次确认", s_need_auth_hits);
            sleep_ms(1000, true);
            return RUN_SUCCESS;
        }
        s_need_auth_hits = 0;

        /**
         * 登录频率保护
         *
         * 60 秒内已经登录过 2 次, 却再次被要求认证 —— 说明认证虽然"成功"但立刻
         * 又不生效, 基本可判定配置有问题(或账号已被限制)。继续试下去服务端会锁
         * 30 分钟, 所以暂停自动认证, 等 35 分钟(服务端锁 30 分钟, 不能卡点)。
         */
        {
            const uint64_t now_ms = get_steady_tm_ms();
            uint8_t recent = 0;
            for (uint8_t i = 0; i < 2; i++)
            {
                if (s_login_ms[i] != 0 && now_ms - s_login_ms[i] < 60000) recent++;
            }

            if (recent >= 2)
            {
                LOG_ERROR("配置 %" PRIu8 " 在 60 秒内已登录 %" PRIu8 " 次仍被要求认证, 判定为配置错误或账号被限制, 暂停自动认证",
                    g_prog_status[tl_thread_idx].login_cfg.idx, recent);

                s_login_ms[0] = 0;
                s_login_ms[1] = 0;
                (void)wait_auth_retry(35); // 服务端锁定 30 分钟, 多等 5 分钟避开卡点
                return RUN_SUCCESS;
            }
        }

        if (g_prog_status[tl_thread_idx].runtime_status.is_initialized) // 进入认证流程的时候如果会话已经初始化, 重置认证配置参数
        {
            reset();
            g_prog_status[tl_thread_idx].runtime_status.is_running = true;
        }
        if (auth() != AUTH_SUCCESS)
        {
            if (g_prog_status[tl_thread_idx].runtime_status.is_running == false)
            {
                return RUN_FAILED;
            }

            const runtime_status_t* status = &g_prog_status[tl_thread_idx].runtime_status;
            const auth_error_kind_t kind = (auth_error_kind_t)status->last_error_kind;
            if (status->last_error_msg[0] != '\0')
            {
                LOG_ERROR("配置 %" PRIu8 " 认证失败: %s (Error-Code: %s, Sub-Error: %s, 分类: %s)",
                    g_prog_status[tl_thread_idx].login_cfg.idx,
                    status->last_error_msg,
                    status->last_error_code[0] != '\0' ? status->last_error_code : "-",
                    status->last_sub_error[0] != '\0' ? status->last_sub_error : "-",
                    auth_error_kind_name(kind));
            }
            else
            {
                LOG_ERROR("配置 %" PRIu8 " 认证失败: 没拿到服务端错误码, 按网络问题处理",
                    g_prog_status[tl_thread_idx].login_cfg.idx);
            }

            uint64_t wait_minutes = 0;
            if (kind == AUTH_ERROR_FATAL)
            {
                /**
                 * 密码错 / 账号不存在 / 版本被停用 / 被限制共享 这类错误, 重试多少次都是同一个结果,
                 * 反而会因为反复失败触发服务端的登录频率限制, 所以停下来等用户处理:
                 * 网页上"保存并应用"(重启)或退出程序都会把这里的等待打断
                 */
                LOG_WARN("这类错误需要人工处理, 已暂停自动重试; 改好配置并保存(会重启)或退出程序后才会继续");
            }
            else
            {
                const size_t table_len = sizeof(table) / sizeof(table[0]);
                const size_t step = (kind == AUTH_ERROR_SLOW)
                                        ? table_len
                                        : (retry_auth <= table_len ? retry_auth : table_len);
                wait_minutes = table[step - 1];
                LOG_WARN("配置 %" PRIu8 " 将在 %" PRIu64 " 分钟后重试 (第 %" PRIu8 " 次失败, 不设上限, 重启或退出可立即打断)",
                    g_prog_status[tl_thread_idx].login_cfg.idx, wait_minutes, retry_auth);
                if (retry_auth < UINT8_MAX) retry_auth++;
            }

            if (wait_auth_retry(wait_minutes) == false)
            {
                LOG_DEBUG("等待被重启/退出请求打断, 本轮认证结束");
            }
            return RUN_SUCCESS;
        }
        retry_auth = 1; // 认证成功: 下次失败重新从最短间隔开始

        // 记下这次登录时间, 供上面的频率保护判断 (两个槽轮转, 只看最近两次)
        s_login_slot ^= 1;
        s_login_ms[s_login_slot] = get_steady_tm_ms();
        return RUN_SUCCESS;
    case STATUS_ERROR: // 网络错误
        retry_auth = 1;
        if (retry_timeout > 5)
        {
            LOG_ERROR("超过最多重试次数");
            return RUN_FAILED;
        }
        LOG_WARN("网络错误, 等待 10 秒后重试, 重试: 第 %" PRIu8 " 次, 最多 5 次",
            retry_timeout);
        sleep_ms(10000, true);
        retry_timeout++;
        return TIMEOUT_RETRY;
    default:
        retry_timeout = 1;
        retry_auth = 1;
        LOG_WARN("网络错误");
        sleep_ms(5000, true);
        return RUN_FAILED;
    }
}

int dialer_app(void* arg)
{
    tl_thread_idx = (int8_t)(intptr_t)arg; // 领取线程下标参数
    g_prog_status[tl_thread_idx].runtime_status.is_running = true;
    g_prog_status[tl_thread_idx].thread_id = sim_thread_cur_id(); // 获取当前线程 TID
    LOG_DEBUG("认证线程 %" PRId8 " 创建成功, ID: %" PRIu64 ", 使用配置: %" PRIu8,
        tl_thread_idx,
        g_prog_status[tl_thread_idx].thread_id,
        g_prog_status[tl_thread_idx].login_cfg.idx);

    refresh_states(); // 刷新数据 (algo_id, host_name, client_id, mac_addr)
    if (get_last_location() == false) g_prog_status[tl_thread_idx].runtime_status.is_running = false;  // 获取 last_location, 用于获取认证配置

    /**
     * 运行循环
     * is_running 为真且 is_need_reset 为假时保持循环
     * 正在运行且不需要重置时保持循环
     * 如果不运行, 或者需要重置时退出循环
     */
    int exit_code = 0;
    while (g_prog_status[tl_thread_idx].runtime_status.is_running && g_stop_requested == 0)
    {
        /**
         * 打卡: 只作为兜底。
         *
         * 真正精确的打卡在两个地方 —— sleep_ms() 里 (每次睡眠按自己的时长打卡)
         * 和 NetClient 的 get()/post() 里 (每次网络请求按连接+操作超时打卡)。
         * 所有可能长时间阻塞的操作都从这两处过, 所以这里只要保证"一轮里
         * 没被覆盖到的部分不会拖太久"就够了。
         */
        watchdog_pet_network();

        if (supervisor_gone())
        {
            LOG_WARN("守护进程已退出, 本进程一并退出");
            break;
        }
        /**
         * 时间控制: 定时线程负责到点触发 (见 work_auth 里的 time_control_init),
         * 这里每轮再补一次 —— sync 是幂等的, 只是让刚好走到这里的那一拍
         * 也用上最新状态
         */
        if (g_prog_role == ROLE_AUTH)
        {
            time_control_sync();
        }

        const RunStatus run_status = run();
        // 如果 run 函数返回 RUN_FAILED 或需要重置, 则退出循环
        if (run_status == RUN_FAILED)
        {
            LOG_ERROR("线程出现错误, 正在退出");
            g_prog_status[tl_thread_idx].runtime_status.is_running = false;
            exit_code = 1; // 认证进程据此退出, 交给外部监管者重新拉起
            break;
        }
        if (g_prog_status[tl_thread_idx].runtime_status.is_need_reauth)
        {
            LOG_INFO("线程需要重置, 正在退出");
            g_prog_status[tl_thread_idx].runtime_status.is_running = false;
            break;
        }
    }

    /**
     * 线程退出时的操作
     */
    watchdog_stop(); // 关闭流程里主循环不再打卡, 别把它当成卡死
    clean(); // 清除参数
    return exit_code;
}

/**
 * @brief 打印程序信息
 */
void print_banner(void)
{
    LOG_INFO("-------------------------------------------------------------------");
    LOG_INFO(" - 程序版本: " PROGRAM_FULL_VERSION);
    LOG_INFO(" - 本程序由 BadGhost 制作, 遵循 Apache-2.0 开源协议");
    LOG_INFO(" - 项目地址: https://github.com/BadGhost520/ESurfingClient-CVersion");
    LOG_INFO("-------------------------------------------------------------------");
}

#ifndef __OPENWRT__

/**
 * @brief 带着网页停下来等用户把配置改好
 *
 * 网页上的"保存并应用"会叫醒这里的循环; 一直改不好时收到退出请求就交回调用方
 * @return true 配置已可用, false 期间收到退出请求
 */
static bool wait_for_cfg(void)
{
    LOG_WARN("配置暂不可用, 请在网页界面上改好后点「保存并应用」");
    LOG_INFO("配置文件路径: %s", get_config_path());

    g_waiting_cfg = true;
    while (g_stop_requested == 0 && g_need_exit == false)
    {
        if (wait_cfg_usable()) break;
    }
    g_waiting_cfg = false;

    return g_cfg_loaded;
}

#endif

/**
 * @brief 配置不可用时停住等用户改 (桌面端) 或者退出交给外部监管者 (OpenWrt)
 *
 * @return true 表示配置已经可用了, false 表示已经/正在退出, 调用方不必再往下走
 */
static bool hold_for_cfg(void)
{
#ifdef __OPENWRT__

    // OpenWrt 上没有网页可以等, 退出交给 procd 按 respawn 策略处理
    shut(1);
    return false;

#else

    return wait_for_cfg();

#endif
}

#ifndef __OPENWRT__

/**
 * @brief Web 进程主流程 (只提供网页服务, 不参与认证)
 *
 * 认证状态与"重新认证 / 应用新配置"都通过控制通道交给认证进程
 * @return 进程退出码
 */
static int work_web(void)
{
    if (app_bootstrap() == false) return 1;

    /**
     * 顺序与认证进程一致: 先加载配置 (即使不可用, Web 服务的端口也已经就位),
     * 再把网页起起来, 配置不可用时就停在这里等用户改。
     *
     * 不能一见配置不可用就退出: 监管者只会按退避策略反复拉起, 而用户需要的
     * 恰恰是这个网页 —— 它是把配置改回来的入口
     */
    const bool cfg_ok = load_cfg();

    if (start_web_server_remote() == false) shut(1);

    // 网页上的卡片: 实时网速与联网状态由本进程的采样/探测线程提供, 热点开关按需调用
    net_traffic_start();
    net_probe_start();
    hotspot_control_start();

    if (cfg_ok == false)
    {
        if (hold_for_cfg() == false) shut(0);
    }

    LOG_DEBUG("以 Web 进程运行, 展示配置 %" PRIu8, g_prog_status[0].login_cfg.idx);

    // Web 服务在独立线程里跑, 主线程只等退出
    while (g_need_exit == false && g_stop_requested == 0)
    {
        if (supervisor_gone())
        {
            LOG_WARN("守护进程已退出, 本进程一并退出");
            break;
        }

        /**
         * 跟着配置文件走: 用户在网页上保存过配置之后, 本进程展示的值也要跟上
         * (认证与"应用配置"是认证进程那边的事, 这里只是把配置重新读一遍);
         * 配置变得不可用时回到"等配置"状态 —— 这样网页上的"保存并应用"就能像
         * 启动时那样直接把配置重新读起来, 而不是回一句"认证进程不可达"
         */
        if (wait_cfg_usable() == false && g_cfg_loaded == false)
        {
            if (hold_for_cfg() == false) shut(0);
        }
    }

    LOG_DEBUG("Web 进程退出");
    shut(0); // 停 Web 服务线程、收尾日志, 不会返回
    return 0;
}

#endif

/**
 * @brief 抢"单实例锁"
 *
 * 同类型实例只能有一个:
 * - 认证进程按账号 (auth-N) —— 两个同账号进程同时认证会互相把对方踢下线, 只能手动杀进程
 * - supervisor / standalone 用 main
 * - Web 子进程用 web
 *
 * POSIX 用 /tmp 下的锁文件 + flock, Windows 用命名互斥; fd / 句柄都故意不关,
 * 交给进程退出时释放。Windows 重启时旧进程还没退干净, 所以重试等最多 3 秒。
 *
 * @param key 锁的 key
 * @return 是否抢到 (false 表示已有同类型实例)
 */
static bool single_instance_acquire(const char* key)
{
#ifdef _WIN32
    char narrow[128];
    snprintf(narrow, sizeof(narrow), "Local\\ESurfingClient-%s", key);

    wchar_t name[128];
    if (MultiByteToWideChar(CP_UTF8, 0, narrow, -1, name, 128) <= 0) return true;

    for (int i = 0; i < 15; i++)
    {
        HANDLE mutex = CreateMutexW(NULL, FALSE, name);
        if (mutex == NULL) return true; // 建不出来就别拦着程序
        if (GetLastError() != ERROR_ALREADY_EXISTS) return true; // 抢到 (句柄故意不关)

        CloseHandle(mutex);
        Sleep(200);
    }
    return false;
#else
    char path[128];
    snprintf(path, sizeof(path), "/tmp/esurfingclient-%s.lock", key);

    /**
     * O_CLOEXEC 必须带: supervisor 拉起的 auth 子进程是 exec 出来的, 继承 fd 会把
     * main 锁一起带过去 —— supervisor 退出后子进程还持有它, 下次启动就抢不到了
     */
    static int lock_fd = -1;
    lock_fd = open(path, O_CREAT | O_RDWR | O_CLOEXEC, 0644);
    if (lock_fd < 0) return true; // 打不开锁文件就别拦着程序

    return flock(lock_fd, LOCK_EX | LOCK_NB) == 0;
#endif
}

void work(void)
{
    /**
     * 单实例检查放在最前面: 同一账号的两个进程会互相踢下线且无法自愈,
     * 重复启动必须先拦住。日志系统这时还没起来, 所以只写 stderr。
     */
    char lock_key[32];
    if (g_prog_role == ROLE_AUTH)
    {
        snprintf(lock_key, sizeof(lock_key), "auth-%" PRIu8, g_prog_account);
    }
    else if (g_prog_role == ROLE_WEB)
    {
        snprintf(lock_key, sizeof(lock_key), "web");
    }
    else
    {
        snprintf(lock_key, sizeof(lock_key), "main");
    }

    if (single_instance_acquire(lock_key) == false)
    {
        fprintf(stderr, "[ERROR] 已有同类型的实例在运行 (%s), 本次退出\n", lock_key);
        exit(1);
    }

    /**
     * 各角色走各自的流程
     */
    if (g_prog_role == ROLE_AUTH)
    {
        exit(work_auth());
    }

#ifndef __OPENWRT__

    /**
     * 桌面端只有"多进程"这一种形态: 不带参数直接运行, 与 `--role supervisor`
     * 走的是同一条路 —— 由监管进程拉起 N 个认证子进程和一个 Web 子进程。
     *
     * 原来的"单进程多线程"路径已经删除: 同一个流程写两遍, 时间控制、配置变更、
     * 退出收尾迟早会各偏一套。角色在这里归一化, 而不是在解析参数时改 ——
     * --list-accounts / --print-log-dir 正是靠"没有显式指定角色"判断能用,
     * 那两个查询在 main() 里就已经返回了。
     */
    if (g_prog_role == ROLE_STANDALONE)
    {
        g_prog_role = ROLE_SUPERVISOR;
    }

    if (g_prog_role == ROLE_SUPERVISOR)
    {
        exit(work_supervisor());
    }

    if (g_prog_role == ROLE_WEB)
    {
        exit(work_web());
    }

#else

    /**
     * OpenWrt 上没有监管进程 (procd 按账号各起一个 `--role auth` 实例),
     * 不带参数手工运行时仍是"单实例多线程": 一个进程带所有账号的认证线程。
     * 这段逻辑只编进 OpenWrt 构建, 桌面端走不到。
     */
    if (app_bootstrap() == false) return;

    /**
     * 先加载配置
     *
     * 配置不可用时不在这里退出: 由下面的 hold_for_cfg() 决定 —— OpenWrt 上没有
     * 网页可以等, 它会直接退出, 交给 procd 按 respawn 策略重新拉起
     */
    const bool cfg_ok = load_cfg(); // 加载配置文件

    // 配置不可用: OpenWrt 上没有网页可以等, 退出交给 procd 按 respawn 策略处理
    if (cfg_ok == false)
    {
        if (hold_for_cfg() == false) shut(0);
    }

    time_control_sync(); // 冷启动时先按当前时间同步各账号的时间控制状态
    if (time_control_init() == false) shut(1); // 启动时间控制定时线程

    logout_previous_session(); // 上一次被强杀时留下的会话, 先补做登出

    /**
     * 预认证阶段: 等网络进入"需要认证"的状态
     *
     * 写成循环是为了配置: 这时还没有认证线程, 网页上的"保存并应用"没人接 ——
     * 用户把程序关掉之后要能停在这里, 而不是拿着旧配置一直往下走
     */
    while (true)
    {
        const WaitResult wait_result = wait_need_auth();

        if (wait_result == WAIT_CFG_CHANGED)
        {
            LOG_INFO("配置已变更, 重新加载");

            if (load_cfg() == false)
            {
                if (hold_for_cfg() == false) shut(0);
            }
            else
            {
                /**
                 * 还没有线程在跑, "需要重新认证"这个请求没有对象, 清掉 ——
                 * 留着的话线程刚起来就会立刻退出一次
                 */
                for (uint8_t i = 0; i < g_prog_cnt; i++)
                {
                    g_prog_status[i].runtime_status.is_need_reauth = false;
                }
            }

            time_control_sync(); // 换了配置, 各账号的时段状态重新算一遍
            continue;
        }

        if (wait_result == WAIT_RETRY_EXHAUSTED) shut(1);
        break;
    }

    // 等待期间收到退出请求
    if (g_stop_requested) shut(0);

    /**
     * 根据配置数创建相应数量的线程
     */
    LOG_DEBUG("开始创建认证线程");
    for (uint8_t i = 0; i < g_prog_cnt; i++)
    {
        if (g_prog_status[i].runtime_status.is_time_disabled)
        {
            LOG_INFO("配置 %" PRIu8 " 当前不在允许时段，暂不启动认证线程", g_prog_status[i].login_cfg.idx);
            continue;
        }
        g_prog_status[i].thread = sim_thread_create_retry(dialer_app, (void*)(intptr_t)i, 5);
        if (g_prog_status[i].thread == NULL)
        {
            LOG_FATAL("认证线程 %" PRIu8 " 创建失败, 退出程序", i);
            shut(1);
        }
    }

    /**
     * 线程守护
     * 登录时间检测
     */
    sleep_ms(5000, false);
    LOG_DEBUG("线程守护开启");
    uint64_t check_time = 0;
    while (g_thread_keep_alive && g_stop_requested == 0)
    {
        if (check_time > 299999)
        {
            check_time = 0;
            LOG_DEBUG("线程守护持续运行中");
        }
        for (uint8_t i = 0; i < g_prog_cnt; i++)
        {
            /**
             * 认证时间超过 172200000 毫秒 (1 天 23 时 50 分) 自动重启认证
             *
             * 同样是单调钟: 墙钟被校时前跳的话, 这个判断会立刻成立,
             * 于是每次 NTP 校时都白重启一次认证
             */
            if (get_steady_tm_ms() - g_prog_status[i].auth_cfg.auth_time >= 172200000 && g_prog_status[i].auth_cfg.auth_time != 0)
            {
                LOG_DEBUG("当前时刻 (单调钟): %" PRIu64 ", 已认证: %" PRIu64 " 毫秒",
                    get_steady_tm_ms(),
                    get_steady_tm_ms() - g_prog_status[i].auth_cfg.auth_time);
                LOG_WARN("认证时间超过 172200000 毫秒 (1 天 23 时 50 分), 为避免被远程服务器踢下线, 正在重新进行认证");
                for (uint8_t j = 0; j < g_prog_cnt; j++)
                {
                    g_prog_status[j].runtime_status.is_need_reauth = true;
                    uint8_t retry_wte = 1;
                    while (g_prog_status[j].runtime_status.is_authed)
                    {
                        if (retry_wte > 5)
                        {
                            LOG_ERROR("超过重试次数, 强制退出线程");
                            sim_thread_destroy(g_prog_status[j].thread);
                        }
                        LOG_DEBUG("等待配置 %" PRIu8 " 登出, 下标 %" PRIu8 ", 等待次数: %" PRIu8 ", 最多 5 次", g_prog_status[j].login_cfg.idx, j, retry_wte);
                        retry_wte++;
                        sleep_ms(2000, true);
                    }
                }
            }

            /**
             * 线程守护
             */
            if (g_prog_status[i].runtime_status.is_running == false)
            {
                if (g_prog_status[i].thread != NULL)
                {
                    int result_code = 0;
                    sim_thread_join(g_prog_status[i].thread, &result_code);
                    g_prog_status[i].thread = NULL;
                    LOG_DEBUG("认证线程 %" PRIu8 " 已结束", i);
                }

                /**
                 * 先处理"配置未加载完成"的兜底, 再判要不要拉起线程.
                 * ("保存并应用"现在走重启, 正常不会把 g_cfg_loaded 置假)
                 */
                if (g_cfg_loaded == false)
                {
                    LOG_WARN("配置文件未完成加载, 令所有线程退出并加载");
                    for (uint8_t j = 0; j < g_prog_cnt; j++)
                    {
                        g_prog_status[j].runtime_status.is_running = true;
                        while (g_prog_status[j].runtime_status.is_authed == true)
                        {
                            sleep_ms(100, true);
                        }
                    }

                    /**
                     * 新配置也可能是不可以用的 (用户刚在网页上把程序关掉, 或者把
                     * 账号删了): 那就不能拿着旧配置接着认证, 得回到"等配置"状态 ——
                     * 网页还开着, 用户改好之后点「保存并应用」照样能接着跑
                     */
                    if (load_cfg() == false)
                    {
                        if (hold_for_cfg() == false) shut(0);
                    }

                    // 重新加载可能把账号数改小了, 下标要重新对一次
                    if (i >= (uint8_t)g_prog_cnt) break;
                }

                if (g_prog_status[i].runtime_status.is_time_disabled)
                {
                    // 时间控制禁用中，不重启该线程；等时间控制线程在允许时段再放行
                    continue;
                }

                LOG_DEBUG("由于线程守护已开启，将会重新启动认证线程 %" PRIu8, i);
                g_prog_status[i].thread = sim_thread_create_retry(dialer_app, (void*)(intptr_t)i, 5);
                if (g_prog_status[i].thread == NULL)
                {
                    LOG_FATAL("认证线程 %" PRIu8 " 创建失败, 退出程序", i);
                    shut(1);
                }
                while (g_prog_status[i].runtime_status.is_running == false)
                {
                    sleep_ms(100, false);
                }
            }
        }
        sleep_ms(10, false);
        check_time += 10;
    }
    LOG_DEBUG("线程守护已关闭");

    // 收到退出请求: 走正常的关闭流程 (停子线程与 Web 服务、收尾日志、退出)
    if (g_stop_requested) shut(0);

    while (g_thread_keep_alive == false
#ifdef _WIN32
        && get_service_mode() == false
#endif
        )
    {
        sleep_ms(10000, false);
    }

#endif /* __OPENWRT__ */
}
