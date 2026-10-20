#include "core/States.h"

#include "config/Config.h"

#include "ConfigInternal.h"

#include "utils/log/Logger.h"
#include "utils/text/TextUtils.h"

#include "cJSON/cJSON.h"

#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#define WINDOWS_UA "CCTP/WinSVR5/1068"
#define LINUX_UA "CCTP/Linux64/1003"
#define OLD_ANDROID_UA "CCTP/android64_vpn/2093"
#define ANDROID_UA "CCTP/android11_64/2104"
#define IOS_UA "CCTP/iOSdy/4023"
#define MACOS_UA "CCTP/macdy/5019"

/** 通道表: 名字 / 别名 / UA 三者集中一处, 免得"名字→序号"和"序号→UA"两张表各写一遍 */
typedef struct
{
    uint8_t chn;
    const char* name;
    const char* alias;
    const char* ua;
} channel_t;

static const channel_t s_channels[] = {
    {1, "windows", "1", WINDOWS_UA},
    {2, "linux", "2", LINUX_UA},
    {3, "android", "3", ANDROID_UA},
    {4, "ios", "4", IOS_UA},
    {5, "macos", "5", MACOS_UA},
};

#define CHANNEL_COUNT (sizeof(s_channels) / sizeof(s_channels[0]))

static const channel_t* channel_find(const uint8_t chn)
{
    for (size_t i = 0; i < CHANNEL_COUNT; i++)
    {
        if (s_channels[i].chn == chn) return &s_channels[i];
    }
    return NULL;
}

const char* channel_name(const uint8_t chn)
{
    const channel_t* found = channel_find(chn);
    return found != NULL ? found->name : s_channels[2].name; // 默认 android
}

uint8_t parse_channel_json(const cJSON* chn, const uint8_t cfg_no)
{
    if (chn == NULL)
    {
        LOG_WARN("配置 %" PRIu8 " channel 参数不存在, 使用默认通道 3 (Android)", cfg_no);
        return 3;
    }

    if (cJSON_IsNumber(chn))
    {
        const uint8_t value = (uint8_t)chn->valueint;
        if (channel_find(value) != NULL) return value;

        LOG_WARN("配置 %" PRIu8 " channel 参数错误, 使用默认通道 3 (Android)", cfg_no);
        return 3;
    }

    if (cJSON_IsString(chn) && chn->valuestring != NULL)
    {
        const char* value = chn->valuestring;
        for (size_t i = 0; i < CHANNEL_COUNT; i++)
        {
            if (str_ieq(value, s_channels[i].name) || str_ieq(value, s_channels[i].alias))
            {
                return s_channels[i].chn;
            }
        }
        // 这几个别名不在表里 (历史叫法), 仍然认
        if (str_ieq(value, "iphone")) return 4;
        if (str_ieq(value, "mac") || str_ieq(value, "osx")) return 5;
    }

    LOG_WARN("配置 %" PRIu8 " channel 参数错误, 使用默认通道 3 (Android)", cfg_no);
    return 3;
}

void apply_channel_ua(login_cfg_t* cfg, const uint8_t cfg_no)
{
    const channel_t* found = channel_find(cfg->chn);
    if (found == NULL)
    {
        LOG_WARN("配置 %" PRIu8 " channel 参数错误, 使用默认通道 3 (Android)", cfg_no);
        cfg->chn = 3;
        found = channel_find(3);
    }

    LOG_INFO("使用通道 %" PRIu8 ": %s", found->chn, found->name);
    snprintf(cfg->user_agent, USER_AGENT_LEN, "%s", found->ua);
}

int list_accounts(void)
{
    /**
     * stdout 要留给账号列表, 日志一行都不落盘 (全部改写到 stderr).
     *
     * 查询模式必须在 init_logger / load_cfg 之前打开: 这两个调用本身就会写日志,
     * 而写进 run.log 的那几行会被 OpenWrt 的启动脚本当成上一轮运行留下的日志
     * 归档走 (见 set_logger_query_mode 的说明)
     */
    set_logger_query_mode(true);
    set_logger_console(false);

    if (init_logger() == false)
    {
        fprintf(stderr, "[ERROR] 日志系统初始化失败\n");
        return -1;
    }

    /**
     * 复用 load_cfg 的整套校验:
     * 列举出来的账号与真正会被加载的必须完全一致, 否则 init 脚本会起出跑不起来的实例
     */
    s_list_only = true;
    const bool loaded = load_cfg();
    s_list_only = false;

    if (loaded == false)
    {
        // 配置有问题时 load_cfg 已经把原因写到 stderr 了
        return -1;
    }

    for (uint8_t i = 0; i < g_prog_cnt; i++)
    {
        printf("%" PRIu8 "\n", g_prog_status[i].login_cfg.idx);
    }
    fflush(stdout);

    // 这里不调用 clean_logger: 它会把 run.log 改名收尾,
    // 而多实例下 run.log 是所有进程共用的, 每次列举都改名会破坏其它实例的写入
    return g_prog_cnt;
}
