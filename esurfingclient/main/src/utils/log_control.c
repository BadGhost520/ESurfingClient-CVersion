#include "utils/logger_internal.h"

LogLevel get_logger_level()
{
    return s_logger_cfg.lv;
}

void set_logger_level(const LogLevel lv)
{
    if (s_logger_cfg.lv != lv)
    {
        s_logger_cfg.lv = lv;
        LOG_INFO("设置日志等级为 [%s]", get_level_str(lv));
    }
}

bool init_logger()
{
    if (get_log_dir(s_logger_cfg.log_dir) == false)
    {
        fprintf(stderr, "[ERROR] 无法准备日志目录\n");
        return false;
    }
    const uint16_t len = snprintf(s_logger_cfg.log_file, sizeof(s_logger_cfg.log_file), "%s%c%s", safe_str(s_logger_cfg.log_dir), SEP, s_file_name);
    if ((size_t)len >= sizeof(s_logger_cfg.log_file))
    {
        fprintf(stderr, "[ERROR] 日志文件路径太长 (最大 %zu)\n", sizeof(s_logger_cfg.log_file));
        return false;
    }
    if (open_log_file() == false)
    {
        fprintf(stderr, "[ERROR] 无法打开日志文件 %s, 如果是 Linux 系统请使用 sudo 运行程序\n", s_logger_cfg.log_file);
        return false;
    }
    LOG_DEBUG("日志系统初始化完成");
    LOG_DEBUG("日志等级: %s", get_level_str(s_logger_cfg.lv));
    s_logger_inited = true;
    return true;
}

void clean_logger()
{
    LOG_DEBUG("关闭日志系统");

    /**
     * 多进程下 run.log 是所有进程共用的, 退出时的重命名只能由一个进程来做:
     * 否则先退出的进程会把文件改名, 其它进程会继续往一个已改名的文件里写.
     * 因此认证/Web 进程只关闭自己的句柄, 改名交给守护进程 (或单进程模式)
     *
     * ⚠️ OpenWrt 上【没有】守护进程: procd 只跑 --role auth 实例, 而
     *    --list-accounts 也刻意不调用本函数 —— 于是没人改名, run.log 会一直追加。
     *    那边由 init 脚本在启动前归档 (archive_previous_log), procd + init 脚本
     *    在 OpenWrt 上就是那个"守护进程"。改动这里时别忘了那一处。
     */
    const bool need_rename = (g_prog_role != ROLE_AUTH && g_prog_role != ROLE_WEB);

    /**
     * 关句柄这一段也要与写日志串行: 否则某个线程可能刚判完句柄非空,
     * 这里就把它 fclose 掉了 —— 与 rotate() 里那个窗口是同一类问题
     */
    logger_lock();

    if (!s_logger_cfg.file_handle)
    {
        logger_unlock();
        fprintf(stderr, "[ERROR] 日志系统未启动\n");
        return;
    }
    fclose(s_logger_cfg.file_handle);
    s_logger_cfg.file_handle = NULL;

    // 收尾之后句柄已经没了, 再 set_logger_dir 只该记下目录, 不该去搬文件
    s_logger_inited = false;

    if (need_rename == false)
    {
        logger_unlock();
        return;
    }

    if (strlen(s_logger_cfg.log_file) == 0)
    {
        logger_unlock();
        fprintf(stderr, "[ERROR] 日志路径为空\n");
        return;
    }
    char cur_tm[32];
    get_fmt_time(cur_tm, FILE_FORMAT);
    char new_file_name[PATH_MAX];
    snprintf(new_file_name, sizeof(new_file_name), "%s%c%s.log", safe_str(s_logger_cfg.log_dir), SEP, safe_str(cur_tm));
    rename(s_logger_cfg.log_file, new_file_name);

    logger_unlock();
}

void set_logger_console(const bool enabled)
{
    s_console_enabled = enabled;
}

void set_logger_query_mode(const bool enabled)
{
    s_query_mode = enabled;
}
