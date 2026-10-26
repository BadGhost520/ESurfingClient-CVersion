#include "core/States.h"

#include "config/Config.h"

#include "utils/text/TextUtils.h"
#include "utils/log/Logger.h"

#include <ctype.h>

static void reset_host_name(void)
{
    auth_cfg_t* auth_cfg = &g_prog_status[tl_thread_idx].auth_cfg;

    char host_name[16];
    unsigned char host_bytes[10];
    get_rand_bytes(host_bytes, 10);
    host_bytes[0] = host_bytes[0] & 0xFEU;
    sprintf(host_name, "%02x%02x%02x%02x%02x",
    host_bytes[0], host_bytes[1],
    host_bytes[2], host_bytes[3],
    host_bytes[4]);
    LOG_DEBUG("新的主机名: %s", host_name);
    snprintf(auth_cfg->host_name, HOST_NAME_LEN, "%s", safe_str(host_name));
}

static void reset_client_id(void)
{
    char client_id[40];
    unsigned char client_bytes[16];
    get_rand_bytes(client_bytes, 16);
    sprintf(client_id,
        "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        client_bytes[0], client_bytes[1],
        client_bytes[2], client_bytes[3],
        client_bytes[4], client_bytes[5],
        (client_bytes[6] & 0x0F) | 0x40,
        (client_bytes[7] & 0x3F) | 0x80,
        client_bytes[8], client_bytes[9],
        client_bytes[10], client_bytes[11],
        client_bytes[12],client_bytes[13],
        client_bytes[14], client_bytes[15]);
    for (int i = 0; client_id[i]; i++) client_id[i] = (char)tolower((unsigned char)client_id[i]);
    LOG_DEBUG("新的 Client Id: %s", client_id);
    snprintf(g_prog_status[tl_thread_idx].auth_cfg.client_id, CLIENT_ID_LEN, "%s", safe_str(client_id));
}

static void reset_mac_addr(void)
{
    char mac_addr[20];
    unsigned char mac_bytes[6];
    get_rand_bytes(mac_bytes, 6);
    mac_bytes[0] = mac_bytes[0] & 0xFEU;
    sprintf(mac_addr, "%02x:%02x:%02x:%02x:%02x:%02x",
    mac_bytes[0], mac_bytes[1],
    mac_bytes[2], mac_bytes[3],
    mac_bytes[4], mac_bytes[5]);
    LOG_DEBUG("新的 MAC 地址: %s", mac_addr);
    snprintf(g_prog_status[tl_thread_idx].auth_cfg.mac_addr, MAC_ADDR_LEN, "%s", safe_str(mac_addr));
}

void refresh_states(void)
{
    snprintf(g_prog_status[tl_thread_idx].auth_cfg.algo_id, ALGO_ID_LEN, "00000000-0000-0000-0000-000000000000"); // 初始化 algo_id
    reset_host_name(); // 重置 hostname
    reset_client_id(); // 重置 client_id
    reset_mac_addr(); // 重置 mac_address

    /**
     * ostag 按通道取固定值 (Windows 11 / Linux / Android 14 / iOS 17.0 / macOS 14.4),
     * 不再跟随主机名随机生成
     */
    snprintf(g_prog_status[tl_thread_idx].auth_cfg.ostag, OSTAG_LEN, "%s",
             channel_ostag(g_prog_status[tl_thread_idx].cfg.chn));
    LOG_DEBUG("ostag: %s", g_prog_status[tl_thread_idx].auth_cfg.ostag);
}
