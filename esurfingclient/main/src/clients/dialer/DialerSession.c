#include "clients/dialer/DialerInternal.h"

#include "clients/net/NetClient.h"

#include "cipher/CipherInterface.h"

#include "states/States.h"

#include "utils/PlatformUtils.h"
#include "utils/Logger.h"

#include <string.h>

static void clean_session()
{
    LOG_DEBUG("清除会话初始化状态");
    destroy_cipher_factory();
    g_prog_status[tl_thread_idx].runtime_status.is_initialized = 0;
}

bool init_session()
{
    LOG_DEBUG("init_session 函数入口检查, 使用配置: %" PRIu8 ", 下标: %" PRIu8, g_prog_status[tl_thread_idx].login_cfg.idx, tl_thread_idx);

    /**
     * 向 ticket_url POST 获取 ZSM.
     * iOS/macOS 没有本地模块时 Algo-ID 为零 UUID, 首次用空 POST.
     * Android/Linux 仍 POST 全 0 UUID.
     */
    const char* ticket_body = g_prog_status[tl_thread_idx].auth_cfg.algo_id;
    if (g_prog_status[tl_thread_idx].login_cfg.chn == 4 || g_prog_status[tl_thread_idx].login_cfg.chn == 5)
    {
        ticket_body = "";
        LOG_INFO("iOS/macOS 通道首次拉取 ZSM 使用空 POST");
    }
    const curl_resp_t resp = post(g_prog_status[tl_thread_idx].auth_cfg.ticket_url, ticket_body);
    if (resp.http_code != HTTP_OK || resp.body_size == 0 || resp.body_data == NULL) // 响应错误或无响应数据, 则返回 false
    {
        LOG_ERROR("初始化会话失败");
        free(resp.body_data);
        return false;
    }
    LOG_DEBUG("会话响应长度: %zu", resp.body_size);
    {
        const bytes_t zsm = {
            .data = (uint8_t*)resp.body_data,
            .length = resp.body_size
        };

        LOG_DEBUG("开始初始化会话");

        /**
         * 加载加解密工厂
         * 如果失败, 返回 false
         */
        if (load_cipher(zsm) == false)
        {
            LOG_DEBUG("初始化会话失败");
            g_prog_status[tl_thread_idx].runtime_status.is_initialized = 0;
            free(resp.body_data);
            return false;
        }
    }
    LOG_DEBUG("初始化会话成功");
    g_prog_status[tl_thread_idx].runtime_status.is_initialized = 1;
    free(resp.body_data);
    return true;
}

void clean()
{
    // 时间控制禁用状态是跨线程的“外部闸门”，线程退出清理时不能把它清掉，
    // 否则线程守护会立刻把刚下线的账号重新拉起来。
    const bool time_disabled = g_prog_status[tl_thread_idx].runtime_status.is_time_disabled;

    if (g_prog_status[tl_thread_idx].runtime_status.is_initialized == true) // 如果已经初始化会话, 则进入
    {
        if (g_prog_status[tl_thread_idx].runtime_status.is_authed == true) // 如果已经认证, 则进入
        {
            LOG_INFO("配置 %" PRIu8 " 登出, 下标: %" PRId8,
                g_prog_status[tl_thread_idx].login_cfg.idx,
                tl_thread_idx);
            term(); // 登出
        }
        clean_session(); // 清理会话
    }
    memset(&g_prog_status[tl_thread_idx].auth_cfg, 0, sizeof(auth_cfg_t)); // 清除 auth_cfg 的内容, 并置零
    memset(&g_prog_status[tl_thread_idx].runtime_status, 0, sizeof(runtime_status_t)); // 清除 runtime_status 的内容, 并置零
    g_prog_status[tl_thread_idx].runtime_status.is_time_disabled = time_disabled; // 恢复时间控制禁用状态
}

void reset()
{
    clean(); // 清理数据
    refresh_states(); // 重置指定数据
}

/**
 * @brief 监管进程是否已经不在了
 *
 * 只在被监管的角色下判断: 单进程模式的父进程是终端或服务管理器,
 * 它们退出并不意味着本程序该退出。
 * Linux 上子进程登记了 PR_SET_PDEATHSIG, 内核会直接通知, 这里恒为 false
 */
bool supervisor_gone()
{
    if (g_prog_role != ROLE_AUTH && g_prog_role != ROLE_WEB) return false;
    return parent_process_alive() == false;
}
