#include "utils/PlatformUtils.h"
#include "utils/Logger.h"
#include "States.h"

#include <ctype.h>

uint64_t g_start_run_tm = 0;

int8_t g_prog_cnt = 0;

_Thread_local int8_t thread_idx = -1;

bool thread_keep_alive = false;

ProgStatus* g_prog_status;

char school_network_symbol[SCHOOL_NETWORK_SYMBOL] = {0};

#ifdef __OPENWRT__
bool g_use_cus_if = false;
#endif

static void set_hostname()
{
    char host_name[16];
    unsigned char host_bytes[10];
    get_rand_bytes(host_bytes, 10);
    host_bytes[0] = host_bytes[0] & 0xFEU;
    sprintf(host_name, "%02x%02x%02x%02x%02x",
    host_bytes[0], host_bytes[1],
    host_bytes[2], host_bytes[3],
    host_bytes[4]);
    LOG_DEBUG("新的主机名: %s", host_name);
    snprintf(g_prog_status[thread_idx].auth_cfg.host_name, HOST_NAME_LEN, "%s", safe_str(host_name));
}

static void set_client_id()
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
    snprintf(g_prog_status[thread_idx].auth_cfg.client_id, CLIENT_ID_LEN, "%s", safe_str(client_id));
}

static void set_mac_address()
{
    char mac_address[20];
    unsigned char mac_bytes[6];
    get_rand_bytes(mac_bytes, 6);
    mac_bytes[0] = mac_bytes[0] & 0xFEU;
    sprintf(mac_address, "%02x:%02x:%02x:%02x:%02x:%02x",
    mac_bytes[0], mac_bytes[1],
    mac_bytes[2], mac_bytes[3],
    mac_bytes[4], mac_bytes[5]);
    LOG_DEBUG("新的 MAC 地址: %s", mac_address);
    snprintf(g_prog_status[thread_idx].auth_cfg.mac_address, MAC_ADDRESS_LEN, "%s", safe_str(mac_address));
}

void refresh_states()
{
    snprintf(g_prog_status[thread_idx].auth_cfg.algo_id, ALGO_ID_LEN, "00000000-0000-0000-0000-000000000000");
    set_hostname();
    set_client_id();
    set_mac_address();
}
