#include "clients/dialer/DialerInternal.h"

#include "clients/net/NetClient.h"

#include "states/States.h"

#include "utils/PlatformUtils.h"
#include "utils/TimeControl.h"
#include "utils/LogoutState.h"
#include "utils/Shutdown.h"
#include "utils/Watchdog.h"
#include "utils/Logger.h"
#include "utils/sim/SimThread.h"

#include <stdio.h>

#ifndef __OPENWRT__
#include "control/Control.h"
#endif

AuthStatus auth()
{
    LOG_DEBUG("auth 函数入口检查, 使用配置: %" PRIu8 ", 下标: %" PRId8, g_prog_status[tl_thread_idx].login_cfg.idx, tl_thread_idx);

    const char portal_start_tag[] = "<!--//config.campus.js.chinatelecom.com";
    const char portal_end_tag[] = "//config.campus.js.chinatelecom.com-->";

    const curl_resp_t resp = get(g_prog_status[tl_thread_idx].last_location, false); // curl GET last_location 获取认证配置
    if (resp.http_code != HTTP_OK || resp.body_size == 0 || resp.body_data == NULL) // 如果响应体没有内容 (非 200 响应码), 则返回
    {
        LOG_ERROR("响应体为空, 无法提取认证配置");
        return AUTH_FAILED;
    }

    char* portal_config = extract_between_tags(resp.body_data, portal_start_tag, portal_end_tag); // 从响应体内容中提取指定内容
    free(resp.body_data);
    if (portal_config == NULL)
    {
        LOG_ERROR("提取门户配置失败");
        return AUTH_FAILED;
    }

    char* auth_url = xml_parser(portal_config, "auth-url"); // 提取 auth_url (包含 CDATA 等字符)
    if (auth_url == NULL)
    {
        LOG_ERROR("提取 Auth URL 失败");
        return AUTH_FAILED;
    }

    char* cleaned_auth_url = clean_CDATA(auth_url); // 提取 auth_url (纯 url, 用于登录函数)
    free(auth_url);
    if (cleaned_auth_url == NULL)
    {
        LOG_ERROR("清除 Auth URL 失败");
        return AUTH_FAILED;
    }
    snprintf(g_prog_status[tl_thread_idx].auth_cfg.auth_url, AUTH_URL_LEN, "%s", safe_str(cleaned_auth_url)); // 将 auth_url 填入认证配置变量中
    LOG_INFO("Auth URL: %s", g_prog_status[tl_thread_idx].auth_cfg.auth_url);
    free(cleaned_auth_url);

    char* ticket_url = xml_parser(portal_config, "ticket-url"); // 提取 ticket_url (包含 CDATA 等字符)
    free(portal_config);
    if (ticket_url == NULL)
    {
        LOG_ERROR("提取 Ticket URL 失败");
        return AUTH_FAILED;
    }

    char* cleaned_ticket_url = clean_CDATA(ticket_url); // 提取 ticket_url (纯 url, 用于获取 ticket)
    free(ticket_url);
    if (cleaned_ticket_url == NULL)
    {
        LOG_ERROR("清除 Ticket URL CDATA 失败");
        return AUTH_FAILED;
    }
    snprintf(g_prog_status[tl_thread_idx].auth_cfg.ticket_url, TICKET_URL_LEN, "%s", safe_str(cleaned_ticket_url)); // 将 ticket_url 填入认证配置变量中
    LOG_INFO("Ticket URL: %s", g_prog_status[tl_thread_idx].auth_cfg.ticket_url);

    char* client_ip = extract_url_param(cleaned_ticket_url, "wlanuserip"); // 提取 client_ip
    if (client_ip == NULL)
    {
        LOG_ERROR("提取 Client IP 失败");
        return AUTH_FAILED;
    }
    snprintf(g_prog_status[tl_thread_idx].auth_cfg.client_ip, IP_LEN, "%s", safe_str(client_ip)); // 将 client_ip 填入认证配置变量中
    LOG_INFO("Client IP: %s", g_prog_status[tl_thread_idx].auth_cfg.client_ip);
    free(client_ip);

    char* ac_ip = extract_url_param(cleaned_ticket_url, "wlanacip"); // 提取 ac_ip
    free(cleaned_ticket_url);
    if (ac_ip == NULL)
    {
        LOG_ERROR("提取 AC IP 失败");
        return AUTH_FAILED;
    }
    snprintf(g_prog_status[tl_thread_idx].auth_cfg.ac_ip, IP_LEN, "%s", safe_str(ac_ip)); // 将 ac_ip 填入认证配置变量中
    LOG_INFO("AC IP: %s", g_prog_status[tl_thread_idx].auth_cfg.ac_ip);
    free(ac_ip);

    /**
     * 初始化会话
     * 如果失败, 清除会话并返回 INIT_SESSION_FAILED
     */
    if (init_session() == false)
    {
        LOG_FATAL("初始化会话失败");
        return INIT_SESSION_FAILED;
    }
    LOG_DEBUG("初始化会话完成");

    /**
     * 获取 ticket
     * 如果失败, 清除会话并返回 GET_TICKET_FAILED
     */
    if (get_ticket() == false)
    {
        LOG_FATAL("获取 Ticket 失败");
        return GET_TICKET_FAILED;
    }
    LOG_DEBUG("完成获取 Ticket");

    /**
     * 登录认证
     * 如果失败, 清除会话并返回 LOGIN_FAILED
     */
    if (login() == false)
    {
        LOG_ERROR("登录失败");
        return LOGIN_FAILED;
    }
    LOG_DEBUG("完成登录");

    g_prog_status[tl_thread_idx].auth_cfg.tick = get_cur_tm_ms(); // 获取当前 tick
    g_prog_status[tl_thread_idx].auth_cfg.auth_time = get_cur_tm_ms(); // 获取认证时间
    LOG_DEBUG("登录时间戳: %" PRIu64, g_prog_status[tl_thread_idx].auth_cfg.auth_time);

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
 * 一个进程只负责一个配置 (由 --account 指定), 与单进程模式的区别:
 * - 不启动 Web 服务器
 * - 不启动线程守护: 认证失败直接退出, 由外部监管者 (procd / systemd / 服务管理器) 重新拉起.
 *   崩溃回环保护交给监管者, 比这里的无限重启可靠
 * - 时间控制由认证线程自己在循环里校正, 不需要单独的定时线程
 * @return 进程退出码
 */
int work_auth()
{
    g_thread_keep_alive = true;

    g_prog_status = calloc(1, sizeof(prog_status_t));
    init_shutdown_hook();

    if (init_logger() == false) return 1;

    print_banner();

    if (load_cfg() == false) shut(1);

    if (g_prog_cnt != 1)
    {
        LOG_FATAL("认证进程需要且只需要一个配置, 当前加载了 %" PRId8 " 个, 请检查 --account", g_prog_cnt);
        shut(1);
    }

    LOG_INFO("以认证进程运行, 负责配置 %" PRIu8, g_prog_status[0].login_cfg.idx);

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
            LOG_INFO("配置 %" PRIu8 " 不在允许时段, 等待 %" PRIu64 " 毫秒后重新检查",
                g_prog_status[0].login_cfg.idx, wait_ms);

            // 这是全流程里最长的合法静默 (可能几小时), sleep_ms 会按这个时长打卡
            sleep_ms(wait_ms, true);
            continue;
        }

        /**
         * 启动时先等网络进入需要认证的状态 (与单进程模式一致).
         * 等待过程中若允许时段关闭, wait_need_auth 会把控制权交回来
         */
        if (network_ready == false)
        {
            const WaitResult wait_result = wait_need_auth();

            if (wait_result == WAIT_RETRY_EXHAUSTED) shut(1);
            if (wait_result == WAIT_EXIT) break;
            if (wait_result == WAIT_TIME_CLOSED) continue;

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
            LOG_ERROR("配置 %" PRIu8 " 认证失败, 进程退出, 由外部监管者重新拉起", g_prog_status[0].login_cfg.idx);
            return auth_code;
        }

        /**
         * Web 端请求"应用新配置"时会把 g_cfg_loaded 置为 false,
         * 这里重新加载. 配置里若已经没有本实例负责的账号, load_cfg 会失败并让进程退出,
         * 由外部监管者按 respawn 策略处理
         */
        if (g_cfg_loaded == false)
        {
            LOG_INFO("配置已变更, 重新加载");
            if (load_cfg() == false) shut(1);

            network_ready = false; // 换过配置后重新做网络检测
            continue;
        }

        LOG_INFO("配置 %" PRIu8 " 需要重新认证, 重新开始认证流程", g_prog_status[0].login_cfg.idx);
    }

    LOG_INFO("认证进程退出");
    shut(0); // 停控制通道、收尾日志, 不会返回
    return 0;
}
