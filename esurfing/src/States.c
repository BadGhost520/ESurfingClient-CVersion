#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "headFiles/utils/PlatformUtils.h"
#include "headFiles/utils/Logger.h"
#include "headFiles/Constants.h"
#include "headFiles/States.h"

__thread int thread_index = -1;
ThreadStatus thread_status[MAX_DIALER_COUNT] = {0};
int64_t g_running_time;

static void setRandomClientId()
{
    char* client_id = malloc(37);
    if (!client_id)
    {
        LOG_ERROR("分配内存失败");
        return;
    }
    unsigned char random_bytes[16];
    randomBytes(random_bytes, 16);
    snprintf(client_id, 37,
        "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        random_bytes[0], random_bytes[1],
        random_bytes[2], random_bytes[3],
        random_bytes[4], random_bytes[5],
        (random_bytes[6] & 0x0F) | 0x40,
        (random_bytes[7] & 0x3F) | 0x80,
        random_bytes[8], random_bytes[9],
        random_bytes[10], random_bytes[11],
        random_bytes[12],random_bytes[13],
        random_bytes[14], random_bytes[15]);
    for (int i = 0; client_id[i]; i++) client_id[i] = (char)tolower((unsigned char)client_id[i]);
    LOG_DEBUG("New Client Id: %s", client_id);
    thread_status[thread_index].dialer_context.auth_config.client_id = strdup(client_id);
    free(client_id);
}

static void setRandomMacAddress()
{
    char* mac_address = malloc(18 * sizeof(char));
    if (!mac_address)
    {
        LOG_ERROR("分配内存失败");
        return;
    }
    unsigned char macBytes[6];
    randomBytes(macBytes, 6);
    macBytes[0] = macBytes[0] & 0xFEU;
    sprintf(mac_address, "%02x:%02x:%02x:%02x:%02x:%02x",
    macBytes[0], macBytes[1],
    macBytes[2], macBytes[3],
    macBytes[4], macBytes[5]);
    LOG_DEBUG("New MAC: %s",mac_address);
    thread_status[thread_index].dialer_context.auth_config.mac_address = strdup(mac_address);
    free(mac_address);
}

void refreshStates()
{
    if (thread_status[thread_index].dialer_context.auth_config.mac_address) free(thread_status[thread_index].dialer_context.auth_config.mac_address);
    if (thread_status[thread_index].dialer_context.auth_config.client_id) free(thread_status[thread_index].dialer_context.auth_config.client_id);
    if (thread_status[thread_index].dialer_context.auth_config.school_id) free(thread_status[thread_index].dialer_context.auth_config.school_id);
    if (thread_status[thread_index].dialer_context.auth_config.algo_id) free(thread_status[thread_index].dialer_context.auth_config.algo_id);
    if (thread_status[thread_index].dialer_context.auth_config.ticket) free(thread_status[thread_index].dialer_context.auth_config.ticket);
    if (thread_status[thread_index].dialer_context.auth_config.domain) free(thread_status[thread_index].dialer_context.auth_config.domain);
    if (thread_status[thread_index].dialer_context.auth_config.area) free(thread_status[thread_index].dialer_context.auth_config.area);
    thread_status[thread_index].dialer_context.auth_config.algo_id = strdup("00000000-0000-0000-0000-000000000000");
    setRandomClientId();
    setRandomMacAddress();
}

void setOpt(const Options opt)
{
    LOG_DEBUG("用户名: %s", opt.usr);
    LOG_DEBUG("密码: %s", opt.pwd);
    LOG_DEBUG("通道: %s", opt.chn);
    if (strcmp(opt.chn, "pc") == 0) USER_AGENT = "CCTP/Linux64/1003";
    else if (strcmp(opt.chn, "phone") == 0) USER_AGENT = "CCTP/android64_vpn/2093";
    LOG_DEBUG("UA: %s", USER_AGENT);
    if (thread_status[thread_index].dialer_context.options.usr) free(thread_status[thread_index].dialer_context.options.usr);
    if (thread_status[thread_index].dialer_context.options.pwd) free(thread_status[thread_index].dialer_context.options.pwd);
    if (thread_status[thread_index].dialer_context.options.chn) free(thread_status[thread_index].dialer_context.options.chn);
    thread_status[thread_index].dialer_context.options.usr = opt.usr;
    thread_status[thread_index].dialer_context.options.pwd = opt.pwd;
    thread_status[thread_index].dialer_context.options.chn = opt.chn;
}