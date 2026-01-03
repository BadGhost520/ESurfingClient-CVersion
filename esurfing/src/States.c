#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "headFiles/utils/PlatformUtils.h"
#include "headFiles/utils/Logger.h"
#include "headFiles/States.h"

__thread int thread_index = -1;
ThreadStatus thread_status[MAX_DIALER_COUNT] = {0};
int64_t g_running_time;

static void setHostname()
{
    char host_name[16];
    unsigned char strBytes[10];
    randomBytes(strBytes, 10);
    strBytes[0] = strBytes[0] & 0xFEU;
    sprintf(host_name, "%02x%02x%02x%02x%02x",
    strBytes[0], strBytes[1],
    strBytes[2], strBytes[3],
    strBytes[4]);
    LOG_DEBUG("HOST_NAME: %s", host_name);
    snprintf(thread_status[thread_index].dialer_context.auth_config.host_name, HOST_NAME_LENGTH, "%s", host_name);
}

static void setRandomClientId()
{
    char client_id[40];
    unsigned char random_bytes[16];
    randomBytes(random_bytes, 16);
    snprintf(client_id, 37,
        "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        random_bytes[0], random_bytes[1],
        random_bytes[2], random_bytes[3],
        random_bytes[4], random_bytes[5],
        random_bytes[6] & 0x0F | 0x40,
        random_bytes[7] & 0x3F | 0x80,
        random_bytes[8], random_bytes[9],
        random_bytes[10], random_bytes[11],
        random_bytes[12],random_bytes[13],
        random_bytes[14], random_bytes[15]);
    for (int i = 0; client_id[i]; i++) client_id[i] = (char)tolower((unsigned char)client_id[i]);
    LOG_DEBUG("New Client Id: %s", client_id);
    snprintf(thread_status[thread_index].dialer_context.auth_config.client_id, CLIENT_ID_LENGTH, "%s", client_id);
}

static void setRandomMacAddress()
{
    char mac_address[20];
    unsigned char macBytes[6];
    randomBytes(macBytes, 6);
    macBytes[0] = macBytes[0] & 0xFEU;
    sprintf(mac_address, "%02x:%02x:%02x:%02x:%02x:%02x",
    macBytes[0], macBytes[1],
    macBytes[2], macBytes[3],
    macBytes[4], macBytes[5]);
    LOG_DEBUG("New MAC: %s", mac_address);
    snprintf(thread_status[thread_index].dialer_context.auth_config.mac_address, MAC_ADDRESS_LENGTH, "%s", mac_address);
}

void refreshStates()
{
    snprintf(thread_status[thread_index].dialer_context.auth_config.algo_id, ALGO_ID_LENGTH, "00000000-0000-0000-0000-000000000000");
    setHostname();
    setRandomClientId();
    setRandomMacAddress();
}

void setOpt(const Options opt, const int index)
{
    LOG_DEBUG("用户名: %s", opt.usr);
    LOG_DEBUG("密码: %s", opt.pwd);
    LOG_DEBUG("通道: %s", opt.chn);
    snprintf(thread_status[index].dialer_context.options.usr, USR_LENGTH, "%s", opt.usr);
    snprintf(thread_status[index].dialer_context.options.pwd, PWD_LENGTH, "%s", opt.pwd);
    snprintf(thread_status[index].dialer_context.options.chn, CHN_LENGTH, "%s", opt.chn);
    if (strcmp(opt.chn, "pc") == 0) snprintf(thread_status[index].dialer_context.auth_config.user_agent, USER_AGENT_LENGTH,  "CCTP/Linux64/1003");
    else if (strcmp(opt.chn, "phone") == 0) snprintf(thread_status[index].dialer_context.auth_config.user_agent, USER_AGENT_LENGTH, "CCTP/android64_vpn/2093");
    LOG_DEBUG("UA: %s", thread_status[index].dialer_context.auth_config.user_agent);
}