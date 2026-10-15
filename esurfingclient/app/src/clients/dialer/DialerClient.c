#include "clients/dialer/DialerClient.h"

#include "clients/dialer/DialerInternal.h"

#include "clients/net/NetClient.h"

#include "states/States.h"

#include "utils/PlatformUtils.h"
#include "utils/TimeControl.h"
#include "utils/Shutdown.h"
#include "utils/Watchdog.h"
#include "utils/Logger.h"
#include "utils/sim/SimThread.h"

#ifdef _WIN32
extern bool get_service_mode();
#else
#include <stdlib.h>
#endif

#ifndef __OPENWRT__
#include "supervisor/Supervisor.h"

extern bool start_web_server_remote();
extern bool start_web_server();
#endif

typedef enum
{
    RUN_SUCCESS = 0,
    RUN_FAILED = 1,
    TIMEOUT_RETRY = 2
} RunStatus;

static const uint64_t table[] = {1, 5, 10, 20, 30};

static RunStatus run()
{
    static uint8_t retry_timeout = 1;
    static uint8_t retry_auth = 1;
    static uint64_t retry_auth_time = 0;

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

    switch (check_network_status(true)) // 检测网络状态
    {
    case STATUS_OK: // 正常联网
        retry_timeout = 1;
        retry_auth = 1;
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
                    LOG_INFO("发送心跳包");
                    uint8_t retry_heartbeat = 1;
                    while (heartbeat() == false)
                    {
                        if (retry_heartbeat > 5)
                        {
                            LOG_FATAL("超过最多重试次数");
                            return RUN_FAILED;
                        }
                        LOG_ERROR("配置 %" PRIu8 " 心跳包发送失败, 下标 %" PRIu8 ", 重试: 第 %" PRIu8 " 次, 最多 5 次",
                            g_prog_status[tl_thread_idx].login_cfg.idx,
                            tl_thread_idx,
                            retry_heartbeat);
                        retry_heartbeat++;
                        sleep_ms(1000, true);
                    }
                    LOG_INFO("下一次重试: %" PRIu64 " 秒后",
                        g_prog_status[tl_thread_idx].auth_cfg.keep_retry);
                    g_prog_status[tl_thread_idx].auth_cfg.tick = get_steady_tm_ms(); // 重新给 tick 赋值 (与上面同源, 都用单调钟)
                }
            }
        }
        else
        {
            LOG_INFO("已连接至互联网");
        }
        sleep_ms(1000, false);
        return RUN_SUCCESS;
    case STATUS_NEED_AUTH: // 需要认证
        retry_timeout = 1;
        LOG_INFO("需要认证");
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
            if (retry_auth > 5)
            {
                LOG_FATAL("超过最多重试次数, 请检查账号密码是否正确");
                return RUN_FAILED;
            }
            retry_auth_time = 60000 * table[retry_auth - 1];
            LOG_ERROR("配置 %" PRIu8 " 认证失败, 下标 %" PRIu8 ", 重试: 第 %" PRIu8 " 次, 最多 5 次, 下一次重试时间: %" PRIu64 " 毫秒 (% " PRIu64 " 秒) 后",
                g_prog_status[tl_thread_idx].login_cfg.idx,
                tl_thread_idx,
                retry_auth,
                retry_auth_time,
                retry_auth_time / 1000);
            retry_auth++;
            sleep_ms(retry_auth_time, true);
        }
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
        LOG_ERROR("网络错误");
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
         * 认证进程里没有独立的时间控制线程, 由本线程自己校正时间窗口
         * 单进程模式下由时间控制线程统一校正, 这里不重复做
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
void print_banner()
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
 * 配置不可用时程序不能一走了之 (用户还得靠网页把配置改回来), 也不能死等:
 * 网页上的"保存并应用"会叫醒下面的循环, 用户改好之后自动重新加载并接着往下走。
 * 配置一直改不好时, 收到退出请求 (Ctrl+C / 停止服务) 就交回调用方
 *
 * @return true 表示配置已经可用了, false 表示期间收到了退出请求
 */
static bool wait_for_cfg()
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
static bool hold_for_cfg()
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
 * @brief Web 进程主流程
 *
 * 只提供网页服务, 不参与认证:
 * - 配置由本进程自己读, 因此 /api/getConfigs 一类的接口两种模式下走的是同一段代码
 * - 认证状态与"重新认证 / 应用新配置"通过控制通道交给认证进程
 * - 不启动线程守护与时间控制线程
 * @return 进程退出码
 */
static int work_web()
{
    g_thread_keep_alive = true;

    g_prog_status = calloc(1, sizeof(prog_status_t));
    init_shutdown_hook();

    if (init_logger() == false) return 1;

    print_banner();

    /**
     * 与单进程模式同一套顺序: 先加载配置 (即使不可用, Web 服务的端口也已经就位),
     * 再把网页起起来, 配置不可用时就停在这里等用户改。
     *
     * 不能一见配置不可用就退出: 监管者只会按退避策略反复拉起, 而用户需要的
     * 恰恰是这个网页 —— 它是把配置改回来的入口
     */
    const bool cfg_ok = load_cfg();

    if (start_web_server_remote() == false) shut(1);

    if (cfg_ok == false)
    {
        if (hold_for_cfg() == false) shut(0);
    }

    LOG_INFO("以 Web 进程运行, 展示配置 %" PRIu8, g_prog_status[0].login_cfg.idx);

    // Web 服务在独立线程里跑, 主线程只等退出
    while (g_need_exit == false && g_stop_requested == 0)
    {
        if (supervisor_gone()) break;

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

    LOG_INFO("Web 进程退出");
    shut(0); // 停 Web 服务线程、收尾日志, 不会返回
    return 0;
}

#endif

void work()
{
    /**
     * 各角色走各自的流程
     */
    if (g_prog_role == ROLE_AUTH)
    {
        exit(work_auth());
    }

#ifndef __OPENWRT__

    if (g_prog_role == ROLE_SUPERVISOR)
    {
        exit(work_supervisor());
    }

    if (g_prog_role == ROLE_WEB)
    {
        exit(work_web());
    }

#endif

    g_thread_keep_alive = true;

    g_prog_status = calloc(1, sizeof(prog_status_t)); // 初始化 g_prog_status 指针并分配 1 个空间

    init_shutdown_hook(); // 初始化关闭钩子

    if (init_logger() == false) return; // 初始化日志系统

    print_banner();

    /**
     * 先加载配置, 再起 Web 服务
     *
     * 顺序不能反: 监听端口与是否允许外部访问 (配置里的 web_port / web_external_acc)
     * 要在绑端口之前就位, 反过来就只能按默认端口开出去了 —— load_cfg 把这两个
     * 参数的解析排在所有"配置不可用"的判断之前, 配置有问题时照样能拿到它们。
     *
     * 但 Web 服务也不能等配置加载成功才起: 网页正是用户改配置的入口 (程序被
     * enabled 关掉 / 账号没填 / JSON 写坏时更是唯一的入口), 排在后面的话这些
     * 情况连网页都打不开, 只剩手改 JSON 一条路
     */
    const bool cfg_ok = load_cfg(); // 加载配置文件

#ifndef __OPENWRT__
    if (start_web_server() == false) shut(1); // 启动 Web 服务器线程
#endif

    // 配置不可用: 桌面端带着网页停下来等用户改, OpenWrt 上退出交给 procd
    if (cfg_ok == false)
    {
        if (hold_for_cfg() == false) shut(0);
    }

    time_control_sync(); // 冷启动时先按当前时间同步各账号的时间控制状态
    if (time_control_init() == false) shut(1); // 启动时间控制定时线程

    logout_previous_session(); // 前台模式的强杀登出流程

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
        g_prog_status[i].thread = sim_thread_create(dialer_app, (void*)(intptr_t)i);
        uint8_t retry_ct = 1;
        while (g_prog_status[i].thread == NULL)
        {
            if (retry_ct > 5)
            {
                LOG_FATAL("超过重试次数, 退出程序");
                shut(1);
            }
            LOG_ERROR("认证线程 %" PRIu8 " 创建失败, 重试中, 重试次数: %" PRIu8 ", 最多 5 次", i, retry_ct);
            g_prog_status[i].thread = sim_thread_create(dialer_app, (void*)(intptr_t)i);
            retry_ct++;
        }
    }

    /**
     * 线程守护
     * 登录时间检测
     */
    sleep_ms(5000, false);
    LOG_INFO("线程守护开启");
    uint64_t check_time = 0;
    while (g_thread_keep_alive && g_stop_requested == 0)
    {
        if (check_time > 299999)
        {
            check_time = 0;
            LOG_INFO("线程守护持续运行中");
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
                            LOG_FATAL("超过重试次数, 强制退出线程");
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
                    LOG_INFO("认证线程 %" PRIu8 " 已结束", i);
                }

                /**
                 * 先处理"配置变了" (网页上的"保存并应用"会把 g_cfg_loaded 置为
                 * false), 再判要不要拉起线程。
                 *
                 * 顺序很重要: "不在允许时段"那条分支是 continue, 排在前面的话
                 * 重新加载会被整个跳过去, 程序就一直拿着旧配置跑
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

                LOG_INFO("由于线程守护已开启，将会重新启动认证线程 %" PRIu8, i);
                g_prog_status[i].thread = sim_thread_create(dialer_app, (void*)(intptr_t)i);
                uint8_t retry_ct = 1;
                while (g_prog_status[i].thread == NULL)
                {
                    if (retry_ct > 5)
                    {
                        LOG_FATAL("超过重试次数, 退出程序");
                        shut(1);
                    }
                    LOG_ERROR("认证线程 %" PRIu8 " 创建失败, 重试中, 重试次数: %" PRIu8 ", 最多 5 次", i, retry_ct);
                    g_prog_status[i].thread = sim_thread_create(dialer_app, (void*)(intptr_t)i);
                    retry_ct++;
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
    LOG_INFO("线程守护已关闭");

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
}
