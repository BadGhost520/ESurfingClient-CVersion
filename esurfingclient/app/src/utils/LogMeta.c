#include "utils/LoggerInternal.h"

/**
 * @brief 组装日志里的"是谁写的"这一段: 角色 + 账号 + 进程号
 *
 * 多个进程共用同一份 run.log, 所以每行必须能看出是哪个进程写的。
 * 光有线程号不够 —— 线程号既不告诉你是谁, 在 Linux 上还是 pthread_t 强转出来的
 * 一个巨大地址 (形如 131239767045824), 拿它去 ps 里也对不上。
 * @param buf 输出缓冲
 * @param len 缓冲长度
 */
void get_proc_str(char* buf, const size_t len)
{
#ifdef _WIN32
    const unsigned long proc_id = (unsigned long)GetCurrentProcessId();
#else
    const unsigned long proc_id = (unsigned long)getpid();
#endif

    const char* role;
    switch (g_prog_role)
    {
        case ROLE_AUTH:       role = "auth";       break;
        case ROLE_WEB:        role = "web";        break;
        case ROLE_SUPERVISOR: role = "supervisor"; break;
        default:              role = "standalone"; break;
    }

    /**
     * 认证进程标上账号序号: OpenWrt 上每个账号一个进程, 光看角色分不清是哪个。
     *
     * 用 g_prog_account (即 --account 的值) 而不是 g_prog_status[0].login_cfg.idx:
     * 两者必然相等 (load_cfg 就是按 idx == g_prog_account 挑的), 但 g_prog_status
     * 要等配置加载完才有值 —— 那样"仅加载配置 N"这一行反而标不出账号, 而它恰恰
     * 是最需要标明是哪个账号的一行
     */
    if (g_prog_role == ROLE_AUTH && g_prog_account != 0)
    {
        snprintf(buf, len, "%s#%" PRIu8 " pid=%lu", role, g_prog_account, proc_id);
    }
    else
    {
        snprintf(buf, len, "%s pid=%lu", role, proc_id);
    }
}

/**
 * @brief 组装日志里的"是哪个线程"这一段
 *
 * 优先用线程自己在入口处声明的名字; 单进程模式下每个账号一个认证线程,
 * 那几条线程靠线程 ID 反查配置序号来区分。
 * @param buf 输出缓冲
 * @param len 缓冲长度
 */
void get_thread_str(char* buf, const size_t len)
{
    if (tl_thread_name != NULL)
    {
        snprintf(buf, len, "%s", tl_thread_name);
        return;
    }

    /**
     * 只有单进程模式才需要按线程 ID 反查:
     * 拆分模式下认证逻辑就在主线程上跑, 而 work_auth() 也把主线程登记成了
     * 配置 0 的线程 —— 不加这个限制的话主线程会被标成 auth#1, 名不副实
     */
    if (g_prog_role == ROLE_STANDALONE && g_prog_status != NULL)
    {
        for (uint8_t i = 0; i < g_prog_cnt; i++)
        {
            if (sim_thread_cur_id() == g_prog_status[i].thread_id)
            {
                snprintf(buf, len, "auth#%" PRIu8, g_prog_status[i].login_cfg.idx);
                return;
            }
        }
    }

    snprintf(buf, len, "main");
}
