#include <stdlib.h>
#include <string.h>
#include <io.h>

#include "headFiles/utils/Logger.h"
#include "headFiles/Constants.h"
#include "headFiles/Options.h"

Options opt;

void setOpt(const Options options)
{
    opt.usr = strdup(options.usr);
    free(options.usr);
    opt.pwd = strdup(options.pwd);
    free(options.pwd);
    opt.chn = strdup(options.chn);
    free(options.chn);
    if (strcmp(opt.chn, "pc") == 0)
        USER_AGENT = "CCTP/Linux64/1003";
    else if (strcmp(opt.chn, "phone") == 0)
        USER_AGENT = "CCTP/android64_vpn/2093";
    else
    opt.isDebug = options.isDebug;
    opt.isSmallDevice = options.isSmallDevice;
    LOG_DEBUG("用户名: %s", opt.usr);
    LOG_DEBUG("密码: %s", opt.pwd);
    LOG_DEBUG("通道: %s", opt.chn);
    LOG_DEBUG("UA: %s", USER_AGENT);
    if (opt.isSmallDevice && access("/etc/openwrt_release", F_OK) == 0) LOG_DEBUG("检测到 OpenWrt 环境，小容量设备模式已开启");
}
