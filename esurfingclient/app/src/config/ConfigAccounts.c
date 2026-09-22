#include "utils/PlatformUtils.h"

#include "states/States.h"

#include "config/ConfigInternal.h"

#include "utils/Logger.h"

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

static bool channel_str_eq(const char* a, const char* b)
{
#ifdef _WIN32
    return _stricmp(a, b) == 0;
#else
    return strcasecmp(a, b) == 0;
#endif
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
        const int value = chn->valueint;
        if (value >= 1 && value <= 5)
        {
            return (uint8_t)value;
        }
        LOG_WARN("配置 %" PRIu8 " channel 参数错误, 使用默认通道 3 (Android)", cfg_no);
        return 3;
    }

    if (cJSON_IsString(chn) && chn->valuestring != NULL)
    {
        const char* value = chn->valuestring;
        if (channel_str_eq(value, "windows") || channel_str_eq(value, "1"))
        {
            return 1;
        }
        if (channel_str_eq(value, "linux") || channel_str_eq(value, "2"))
        {
            return 2;
        }
        if (channel_str_eq(value, "android") || channel_str_eq(value, "3"))
        {
            return 3;
        }
        if (channel_str_eq(value, "ios") || channel_str_eq(value, "iphone") || channel_str_eq(value, "4"))
        {
            return 4;
        }
        if (channel_str_eq(value, "macos") || channel_str_eq(value, "mac") || channel_str_eq(value, "osx") || channel_str_eq(value, "5"))
        {
            return 5;
        }
    }

    LOG_WARN("配置 %" PRIu8 " channel 参数错误, 使用默认通道 3 (Android)", cfg_no);
    return 3;
}

void apply_channel_ua(login_cfg_t* cfg, uint8_t cfg_no)
{
    switch (cfg->chn)
    {
    case 1:
        LOG_INFO("使用通道 1: Windows (暂未实现, 使用 Android 通道)");
        snprintf(cfg->user_agent, USER_AGENT_LEN, ANDROID_UA);
        break;
    case 2:
        LOG_INFO("使用通道 2: Linux");
        snprintf(cfg->user_agent, USER_AGENT_LEN, LINUX_UA);
        break;
    case 3:
        LOG_INFO("使用通道 3: Android");
        snprintf(cfg->user_agent, USER_AGENT_LEN, ANDROID_UA);
        break;
    case 4:
        LOG_INFO("使用通道 4: iOS");
        snprintf(cfg->user_agent, USER_AGENT_LEN, IOS_UA);
        break;
    case 5:
        LOG_INFO("使用通道 5: macOS");
        snprintf(cfg->user_agent, USER_AGENT_LEN, MACOS_UA);
        break;
    default:
        LOG_WARN("配置 %" PRIu8 " channel 参数错误, 使用默认通道 3 (Android)", cfg_no);
        cfg->chn = 3;
        snprintf(cfg->user_agent, USER_AGENT_LEN, ANDROID_UA);
        break;
    }
}

int list_accounts()
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
