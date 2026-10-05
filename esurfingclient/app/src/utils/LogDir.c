#include "utils/PlatformUtils.h"

#include "config/ConfigInternal.h"

#include "utils/Logger.h"

#include <stdio.h>

const char* print_log_dir()
{
    /**
     * stdout 要留给路径, 日志一行都不落盘 (全部改写到 stderr).
     *
     * 查询模式必须在 init_logger / load_cfg 之前打开。
     * 这个查询是 OpenWrt 启动脚本在归档上一轮日志【之前】调的, 而它自己会写三行:
     * 没有旧日志时这三行让 run.log 从"不存在"变成"非空", 脚本于是凭空归档出一个
     * 只有查询输出的 .log; 有旧日志时这三行会混进归档里 (见 set_logger_query_mode)
     */
    set_logger_query_mode(true);
    set_logger_console(false);

    if (init_logger() == false)
    {
        fprintf(stderr, "[ERROR] 日志系统初始化失败\n");
        return NULL;
    }

    /**
     * 日志目录由配置里的 log_dir 决定, 只有读完配置才知道到底在哪。
     * 配置有问题时按"列举账号"那套处理: 直接失败, 别挂住 —— 这个查询是给脚本调的
     */
    s_list_only = true;
    const bool loaded = load_cfg();
    s_list_only = false;

    if (loaded == false)
    {
        // 配置有问题时 load_cfg 已经把原因写到 stderr 了
        return NULL;
    }

    return get_logger_dir();
}
