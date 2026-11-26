#include "DialerInternal.h"

#include "dialer/DialerClient.h"

#include "net/NetClient.h"

#include "config/Config.h"

#include "core/States.h"

#include "dialer/ErrorCode.h"


#include "utils/text/TextUtils.h"
#include "utils/time/TimeUtils.h"
#include "utils/time/TimeControl.h"
#include "dialer/LogoutState.h"
#include "utils/platform/PlatformUtils.h"
#include "utils/runtime/Shutdown.h"
#include "utils/runtime/Watchdog.h"
#include "utils/log/Logger.h"
#include "utils/sim/SimThread.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef __OPENWRT__
#include "control/Control.h"
#endif

/** @brief 从 XML 标签提取内容 (支持有/无 CDATA) */
static char* extract_portal_tag(const char* xml, const char* tag)
{
    char* raw = xml_parser(xml, tag);
    if (raw == NULL) return NULL;

    if (strstr(raw, "<![CDATA[") != NULL)
    {
        char* clean = clean_cdata(raw);
        free(raw);
        return clean;
    }
    return raw;
}

AuthStatus auth(void)
{
    LOG_DEBUG("auth 函数入口检查, 使用配置: %" PRIu8 ", 下标: %" PRId8, g_prog_status[tl_thread_idx].cfg.idx, tl_thread_idx);

    // 新一轮尝试: 先把上一次留下的失败原因清掉, 免得没出新码时界面还挂着旧原因
    error_code_record(NULL);

    auth_cfg_t* cfg = &g_prog_status[tl_thread_idx].auth_cfg;

    /**
     * 若上次会话已被清理或尚无重定向地址, 主动刷新一次 last_location
     * (防止路由拨号 DHCP 重分配 IP 后仍拿着旧 wlanuserip)
     */
    if (g_prog_status[tl_thread_idx].last_location_lock == false ||
        g_prog_status[tl_thread_idx].last_location[0] == '\0')
    {
        (void)refresh_last_location();
    }

    /**
     * 1. 先请求门户页面
     *
     * 两个作用:
     * - 拿响应头里的 School Id / Domain / Area, 以及 EConfig 里的集群节点
     *   (<server-clusters> / <auth-url> / <ticket-url>), 纳入后续测速与故障转移池;
     * - 从门户配置块的 ticket-url 里兜底取 wlanuserip / wlanacip。
     */
    curl_resp_t resp = get(g_prog_status[tl_thread_idx].last_location);
    char* portal_ticket_url = NULL;

    if (resp.http_code == HTTP_OK && resp.body_size > 0 && resp.body_data != NULL)
    {
        const char portal_start_tag[] = "<!--//config.campus.js.chinatelecom.com";
        const char portal_end_tag[] = "//config.campus.js.chinatelecom.com-->";

        char* portal_config = extract_between_tags(resp.body_data, portal_start_tag, portal_end_tag);
        if (portal_config != NULL)
        {
            auth_server_extract_econfig(portal_config, g_prog_status[tl_thread_idx].last_location);
            portal_ticket_url = extract_portal_tag(portal_config, "ticket-url");
            free(portal_config);
        }
        else
        {
            auth_server_extract_econfig(NULL, g_prog_status[tl_thread_idx].last_location);
        }
    }
    else
    {
        auth_server_extract_econfig(NULL, g_prog_status[tl_thread_idx].last_location);
        LOG_WARN("门户页面没拿到 (状态码 %ld), 只用 last_location 里的参数", resp.http_code);
    }
    curl_resp_free(&resp);

    /**
     * 2. client_ip / ac_ip: ticket 请求必需
     *
     * last_location 的查询串优先 (302 那一跳通常就带着), 取不到再从门户给的
     * ticket-url 里取。
     */
    char* client_ip = extract_url_param(g_prog_status[tl_thread_idx].last_location, "wlanuserip");
    if (client_ip == NULL && portal_ticket_url != NULL)
    {
        client_ip = extract_url_param(portal_ticket_url, "wlanuserip");
    }
    if (client_ip == NULL)
    {
        LOG_ERROR("提取 Client IP 失败");
        free(portal_ticket_url);
        return AUTH_FAILED;
    }
    snprintf(cfg->client_ip, IP_LEN, "%s", safe_str(client_ip));
    free(client_ip);
    LOG_DEBUG("Client IP: %s", cfg->client_ip);

    char* ac_ip = extract_url_param(g_prog_status[tl_thread_idx].last_location, "wlanacip");
    if (ac_ip == NULL && portal_ticket_url != NULL)
    {
        ac_ip = extract_url_param(portal_ticket_url, "wlanacip");
    }
    if (ac_ip == NULL)
    {
        LOG_ERROR("提取 AC IP 失败");
        free(portal_ticket_url);
        return AUTH_FAILED;
    }
    snprintf(cfg->ac_ip, IP_LEN, "%s", safe_str(ac_ip));
    free(ac_ip);
    LOG_DEBUG("AC IP: %s", cfg->ac_ip);

    free(portal_ticket_url);

    /**
     * 3. gwip: 报文里要填默认网关地址 (不是 AC 地址), 取不到就留空
     */
    if (get_default_gateway_ip(cfg->gwip, sizeof(cfg->gwip)) == false)
    {
        LOG_DEBUG("取不到默认网关地址, gwip 留空");
    }
    else
    {
        LOG_DEBUG("网关地址: %s", cfg->gwip);
    }

    /**
     * 4. 热检查配置文件并按健康度 + TCP 延迟排序候选认证服务器, 逐台自动故障转移
     */
    (void)auth_servers_reload_if_changed();

    char candidates[AUTH_CANDIDATE_MAX][AUTH_SERVER_LEN + 16];
    const size_t cand_count = select_auth_servers(candidates, AUTH_CANDIDATE_MAX);
    if (cand_count == 0)
    {
        LOG_ERROR("没有可用的认证服务器");
        return AUTH_FAILED;
    }

    AuthStatus last_status = AUTH_FAILED;
    bool logged_in = false;

    for (size_t i = 0; i < cand_count; i++)
    {
        if (g_stop_requested != 0 || g_need_exit)
        {
            return AUTH_FAILED;
        }

        const char* server = candidates[i];
        if (i > 0)
        {
            LOG_WARN("认证服务器 %s 失败, 自动切换至备用服务器 %s (%zu/%zu)",
                     candidates[i - 1], server, i + 1, cand_count);
            destroy_cipher_factory();
            g_prog_status[tl_thread_idx].runtime_status.is_initialized = false;
            refresh_states();
            error_code_record(NULL);
        }

        snprintf(cfg->auth_url, AUTH_URL_LEN, "%.*s/auth.cgi", (int)(sizeof(candidates[0]) - 1), server);
        snprintf(cfg->keep_url, KEEP_URL_LEN, "%.*s/keep.cgi", (int)(sizeof(candidates[0]) - 1), server);
        snprintf(cfg->term_url, TERM_URL_LEN, "%.*s/term.cgi", (int)(sizeof(candidates[0]) - 1), server);
        snprintf(cfg->ticket_url, TICKET_URL_LEN, "%.*s/ticket.cgi?wlanuserip=%.*s&wlanacip=%.*s",
                 (int)(sizeof(candidates[0]) - 1), server,
                 IP_LEN - 1, cfg->client_ip,
                 IP_LEN - 1, cfg->ac_ip);

        LOG_DEBUG("Auth URL: %s", cfg->auth_url);
        LOG_DEBUG("Keep-Url: %s", cfg->keep_url);
        LOG_DEBUG("Term-Url: %s", cfg->term_url);
        LOG_DEBUG("Ticket URL: %s", cfg->ticket_url);

        if (init_session() == false)
        {
            LOG_ERROR("初始化会话失败 (%s)", server);
            const auth_error_kind_t kind = (auth_error_kind_t)g_prog_status[tl_thread_idx].runtime_status.last_error_kind;
            if (kind == AUTH_ERROR_FATAL || kind == AUTH_ERROR_SLOW) return INIT_SESSION_FAILED;
            auth_server_mark_failed(server);
            last_status = INIT_SESSION_FAILED;
            continue;
        }
        LOG_DEBUG("初始化会话完成");

        if (get_ticket() == false)
        {
            LOG_ERROR("获取 Ticket 失败 (%s)", server);
            const auth_error_kind_t kind = (auth_error_kind_t)g_prog_status[tl_thread_idx].runtime_status.last_error_kind;
            if (kind == AUTH_ERROR_FATAL || kind == AUTH_ERROR_SLOW) return GET_TICKET_FAILED;
            auth_server_mark_failed(server);
            last_status = GET_TICKET_FAILED;
            continue;
        }
        LOG_DEBUG("完成获取 Ticket");

        if (login() == false)
        {
            LOG_ERROR("登录失败 (%s)", server);
            /**
             * 密码错 / 账号受限 / 终端超限等帐号级错误绝不切换备用节点重试,
             * 避免多台节点连续叠加失败直接触发服务端 30 分钟锁定
             */
            const auth_error_kind_t kind = (auth_error_kind_t)g_prog_status[tl_thread_idx].runtime_status.last_error_kind;
            if (kind == AUTH_ERROR_FATAL || kind == AUTH_ERROR_SLOW)
            {
                return LOGIN_FAILED;
            }
            auth_server_mark_failed(server);
            last_status = LOGIN_FAILED;
            continue;
        }

        auth_server_mark_ok(server);
        logged_in = true;
        break;
    }

    if (logged_in == false)
    {
        return last_status;
    }
    LOG_DEBUG("完成登录");

    /**
     * tick / auth_time 都存【单调钟】而不是墙钟: 它们只被用来做减法测间隔
     * (心跳间隔、会话时长), 而 OpenWrt 上 NTP 校时会把墙钟整体前跳,
     * 墙钟一减就会凭空多出几十小时 —— 表现是"刚登录就提示认证时间超过 1 天 23 时",
     * 或者心跳节律被那一次跳变带乱
     */
    g_prog_status[tl_thread_idx].auth_cfg.tick = get_steady_tm_ms(); // 获取当前 tick
    g_prog_status[tl_thread_idx].auth_cfg.auth_time = get_steady_tm_ms(); // 获取认证时间
    LOG_DEBUG("登录时刻 (单调钟): %" PRIu64, g_prog_status[tl_thread_idx].auth_cfg.auth_time);

    g_prog_status[tl_thread_idx].runtime_status.is_authed = true;
    LOG_INFO("已认证登录");

    /**
     * 把"以后要登出所需要的现场"存下来。
     *
     * 存下来之后, 即使进程被强杀 (断电 / 看门狗 / 端口撞车), 下次启动也能
     * 先把这次会话登出掉 —— 否则账号会挂在服务端在线, 下次只能一直刷
     * "已连接至互联网", 等服务器把它踢下线。登出成功时会把存档删掉。
     */
    logout_state_save(&g_prog_status[tl_thread_idx]);

    sleep_ms(5000, false);
    return AUTH_SUCCESS;
}

/**
 * @brief 认证进程主流程
 *
 * 一个进程只负责一个配置 (由 --account 指定)。桌面端由监管进程拉起多个本进程,
 * OpenWrt 上由 procd 按账号各起一个。这里刻意不启动的东西:
 * - Web 服务器 (那是 Web 子进程的事)
 * - 线程守护: 认证失败直接退出, 由外部监管者 (procd / systemd / 服务管理器) 重新拉起.
 *   崩溃回环保护交给监管者, 比这里的无限重启可靠
 * @return 进程退出码
 */
int work_auth(void)
{
    if (app_bootstrap() == false) return 1;

    if (load_cfg() == false) shut(1);

    if (g_prog_cnt != 1)
    {
        LOG_FATAL("认证进程需要且只需要一个配置, 当前加载了 %" PRId8 " 个, 请检查 --account", g_prog_cnt);
        shut(1);
    }

    LOG_DEBUG("以认证进程运行, 负责配置 %" PRIu8, g_prog_status[0].cfg.idx);

    /**
     * 认证进程只有一个账号, 主线程的探测/请求都按 0 号账号绑 mark。
     *
     * 不设的话主线程 tl_thread_idx 是 -1, open_socket_callback 会当成"没有账号
     * 上下文"而不绑 SO_MARK —— 等待网络阶段的探测就会走主路由表, 看到的是主线路
     * 状态 (多 WAN 下别的线路的会话一死, 本账号也会被误判成需要认证)。
     */
    tl_thread_idx = 0;

    /**
     * 记下本进程的线程 ID, 让日志能标出这是哪个账号的进程
     * 必须放在 load_cfg 之后: 日志标识用的是配置序号
     */
    g_prog_status[0].thread_id = sim_thread_cur_id();

#ifndef __OPENWRT__

    /**
     * 启动控制通道, 供 Web 进程查询状态与下发动作.
     * 端口被占用不算致命: 认证本身不需要它, 降级为不提供远程控制即可
     */
    if (control_server_start(g_control_port) == false)
    {
        /**
         * 说清后果: 控制服务起不来, 监管者停止本进程时就只能用强杀的办法,
         * 那样跑不到 clean(), 也就不会登出 —— 账号会停在服务端在线状态。
         * 最常见的原因是端口被别的程序占了 (监管者按 g_control_port + 账号下标分配)
         */
        LOG_WARN("控制通道启动失败 (端口 %" PRIu16 " 可能被占用), 本实例将不提供远程控制, "
                 "停止时也无法被请求优雅退出 (会被直接结束, 不会登出)", g_control_port);
    }

#endif

    /**
     * 启动看门狗
     *
     * 外部监管者只能看到"进程退出了没有": 进程卡死 (死锁 / 无超时的阻塞调用 /
     * 绕不出来的死循环) 时它照样活着, 监管者永远不会重启它。
     * OpenWrt 上尤其明显 —— 那边连监管进程都没有, 只有 procd 在看进程在不在。
     *
     * 放在配置加载之后: 打卡预算要用配置里的网络超时来算
     */
    watchdog_start();

#ifndef __OPENWRT__

    /**
     * 时间控制定时线程 (仅桌面端)
     *
     * 桌面端统一成多进程之后, 认证进程就是原来那个"主流程": 时段到点该下线时
     * 不能等认证循环下一次跑回 time_control_sync() —— 心跳与失败退避期间
     * 那一等可能是几十秒。定时线程会在窗口边界准时置 is_need_reauth,
     * sleep_ms(..., true) 随即返回, 与原单进程形态一致。
     * 没有账号启用时间控制时它不建线程, 直接返回 true。
     *
     * OpenWrt 的 auth 实例保持原样 (不在这里起线程): 那边仍然由认证循环与
     * wait_need_auth 自己校正, 只有它的单实例模式才用定时线程。
     */
    time_control_sync(); // 冷启动时先按当前时间同步各账号的时间控制状态
    if (time_control_init() == false) shut(1);

#endif

    /**
     * 先把上次没做完的登出补上, 再走正常流程
     *
     * 必须排在网络检测之前: 会话还挂在服务端时, 网络检测会认为"已连接至互联网"
     * 而一直空转, 要等服务器把会话踢下线才能重新认证
     */
    logout_previous_session();

    /**
     * 认证循环
     * dialer_app 内部已处理登录/心跳/登出/重试, 这里只决定"什么时候再跑一轮":
     * - 不在允许时段: 不做任何网络动作, 等下一次时间窗口
     * - 需要重新认证: 立刻重来
     * - 认证失败: 进程退出, 交给外部监管者
     */
    bool network_ready = false;

    while (g_need_exit == false && g_stop_requested == 0)
    {
        if (supervisor_gone())
        {
            LOG_WARN("守护进程已退出, 本进程一并退出");
            break;
        }

        time_control_sync();

        if (g_prog_status[0].runtime_status.is_time_disabled)
        {
            /**
             * 不在允许时段, 此时没有会话需要登出.
             * 必须把这两个标志复位, 否则 sleep_ms 会因为 is_need_reauth 立刻返回造成忙等
             */
            g_prog_status[0].runtime_status.is_running = true;
            g_prog_status[0].runtime_status.is_need_reauth = false;
            network_ready = false; // 换到下一个允许时段后重新做一次网络检测

            const uint64_t wait_ms = time_control_wait_ms();
            LOG_DEBUG("配置 %" PRIu8 " 不在允许时段, 等待 %" PRIu64 " 毫秒后重新检查",
                g_prog_status[0].cfg.idx, wait_ms);

            // 这是全流程里最长的合法静默 (可能几小时), sleep_ms 会按这个时长打卡
            sleep_ms(wait_ms, true);
            continue;
        }

        /**
         * 启动时先等网络进入需要认证的状态。
         * 等待过程中若允许时段关闭, wait_need_auth 会把控制权交回来
         */
        if (network_ready == false)
        {
            const WaitResult wait_result = wait_need_auth();

            if (wait_result == WAIT_RETRY_EXHAUSTED) shut(1);
            if (wait_result == WAIT_EXIT) break;
            if (wait_result == WAIT_TIME_CLOSED) continue;

            /**
             * Web 端请求"应用新配置": 还会在等网络的时候把配置重新读一遍 ——
             * 配置里若已经没有本实例负责的账号, load_cfg 会失败, 进程退出交给
             * 外部监管者 (与下面认证线程跑完那一轮的处置一致)
             */
            if (wait_result == WAIT_CFG_CHANGED)
            {
                LOG_INFO("配置已变更, 重新加载");
                if (load_cfg() == false) shut(1);
                continue;
            }

            network_ready = true;
        }

        const int auth_code = dialer_app((void*)(intptr_t)0);

        /**
         * 收到退出请求 (信号只置了标志): dialer_app 的循环会随之退出,
         * 正常返回并跑完 clean() —— 登出就在这里发生
         */
        if (g_need_exit || g_stop_requested) break;

        if (auth_code != 0)
        {
            LOG_ERROR("配置 %" PRIu8 " 认证失败, 进程退出, 由外部监管者重新拉起", g_prog_status[0].cfg.idx);
            return auth_code;
        }

        /**
         * 兜底: 配置只在启动时读取 (网页上的"保存并应用"已改为重启进程),
         * 所以这里正常情况下不会命中; 若将来有别的路径把 g_cfg_loaded 置假,
         * 重新加载失败就退出, 由监管者按 respawn 策略处理
         */
        if (g_cfg_loaded == false)
        {
            LOG_INFO("配置已变更, 重新加载");
            if (load_cfg() == false) shut(1);

            network_ready = false; // 换过配置后重新做网络检测
            continue;
        }

        LOG_DEBUG("配置 %" PRIu8 " 需要重新认证, 重新开始认证流程", g_prog_status[0].cfg.idx);

        /**
         * dialer_app 可能因为"还没拿到认证页面地址"之类的原因立刻返回
         * (is_running 被置假)。不复位标志、不退避的话, 这里会变成每秒好几轮的
         * 空转 —— 实测 8 秒能写出 500 多行日志。这里复位并隔 5 秒再试。
         */
        if (g_prog_status[0].runtime_status.is_running == false)
        {
            g_prog_status[0].runtime_status.is_running = true;
            sleep_ms(5000, true);
        }
    }

    LOG_DEBUG("认证进程退出");
    shut(0); // 停控制通道、收尾日志, 不会返回
    return 0;
}
