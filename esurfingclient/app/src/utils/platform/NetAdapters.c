#ifdef _WIN32
// GetIfEntry2 / MIB_IF_ROW2 (64 位计数器) 要 Vista 以上的头才会露出来
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#endif

#include "utils/platform/PlatformUtils.h"

#ifdef _WIN32
// 必须排在任何可能带进 windows.h 的头之前
#include <winsock2.h>
// netioapi.h 里的 MIB_IF_ROW2 / GetIfEntry2 (64 位计数器) 要求已经包含过 ws2ipdef.h,
// 而 iphlpapi.h 是通过 __IPHLPAPI_H__ 分支包含 netioapi.h 的、不会替你补上这一头
#include <ws2ipdef.h>
#include <windows.h>
#endif

#include "core/States.h"

#include "PlatformInternal.h"
#include "utils/log/Logger.h"

#ifdef _WIN32
#include <netioapi.h>
#endif

#include "cJSON/cJSON.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    char ip[IP_LEN];
    char name[NAME_LENGTH];
} adapter_t;

static adapter_t* s_adaptor = NULL;
static uint8_t s_adaptor_count = 0;

static void get_adapters()
{
#ifdef _WIN32
    PIP_ADAPTER_INFO p_adapter_info = NULL;
    ULONG ul_out_buf_len = 0;
    if (GetAdaptersInfo(p_adapter_info, &ul_out_buf_len) == ERROR_BUFFER_OVERFLOW)
    {
        p_adapter_info = (PIP_ADAPTER_INFO)malloc(ul_out_buf_len);
        if (p_adapter_info && GetAdaptersInfo(p_adapter_info, &ul_out_buf_len) == NO_ERROR)
        {
            PIP_ADAPTER_INFO p_adapter = p_adapter_info;
            uint8_t cnt = 0;
            while (p_adapter)
            {
                adapter_t* new_adaptor = realloc(s_adaptor, sizeof(adapter_t) * (cnt + 1));
                if (!new_adaptor)
                {
                    LOG_ERROR("分配内存失败");
                    break;
                }
                s_adaptor = new_adaptor;
                snprintf(s_adaptor[cnt].name, NAME_LENGTH, "%s", p_adapter->Description);
                snprintf(s_adaptor[cnt].ip, IP_LEN, "%s", p_adapter->IpAddressList.IpAddress.String);
                LOG_VERBOSE("IP: %s", p_adapter->IpAddressList.IpAddress.String);
                p_adapter = p_adapter->Next;
                cnt++;
            }
            s_adaptor_count = cnt;
        }
    }
    if (p_adapter_info) free(p_adapter_info);
#else
    struct ifaddrs* ifaddrs_ptr, *ifa;
    if (getifaddrs(&ifaddrs_ptr) == 0)
    {
        uint8_t cnt = 0;
        for (ifa = ifaddrs_ptr; ifa; ifa = ifa->ifa_next)
        {
            if (!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET) continue;
            if (strcmp(ifa->ifa_name, "lo") == 0) continue;
            char ip[INET_ADDRSTRLEN];
            struct sockaddr_in *addr = (struct sockaddr_in*)ifa->ifa_addr;
            if (inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip)))
            {
                adapter_t* new_adaptor = realloc(s_adaptor, sizeof(adapter_t) * (cnt + 1));
                if (!new_adaptor)
                {
                    LOG_ERROR("分配内存失败");
                    break;
                }
                s_adaptor = new_adaptor;
                snprintf(s_adaptor[cnt].name, NAME_LENGTH, "%s", ifa->ifa_name);
                snprintf(s_adaptor[cnt].ip, IP_LEN, "%s", ip);
                cnt++;
            }
        }
        s_adaptor_count = cnt;
        freeifaddrs(ifaddrs_ptr);
    }
#endif
}

char* get_adapters_json()
{
    get_adapters();
    cJSON* root = cJSON_CreateObject();
    cJSON* adapters = cJSON_CreateArray();
    for (uint8_t i = 0; i < s_adaptor_count; i++)
    {
        if (strlen(s_adaptor[i].name) == 0) break;
        cJSON* adapter = cJSON_CreateObject();
        cJSON_AddStringToObject(adapter, "name", s_adaptor[i].name);
        cJSON_AddStringToObject(adapter, "ip", s_adaptor[i].ip);
        cJSON_AddItemToArray(adapters, adapter);
    }
    cJSON_AddItemToObject(root, "adapters", adapters);
    cJSON_AddStringToObject(root, "school_network_symbol", g_school_network_symbol);
    char* json = cJSON_Print(root);
    cJSON_Delete(root);
    return json;
}

#ifdef _WIN32

/**
 * @brief 网卡 GUID ("{xxxx-...}") 转成界面用的 id: 小写, 去掉花括号
 *
 * 与 Windows 上 NetworkInterface.Id 的写法保持一致, 网页里存过的"上次选的网卡"
 * 换到这个接口之后还能继续用
 */
static void adapter_id_from_guid(const char* guid, char* out, const size_t out_len)
{
    if (out_len == 0) return;
    size_t n = 0;
    for (const char* p = guid; p != NULL && *p != '\0' && n + 1 < out_len; p++)
    {
        if (*p == '{' || *p == '}') continue;
        char c = *p;
        if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
        out[n++] = c;
    }
    out[n] = '\0';
}

/**
 * @brief UTF-16 转 UTF-8 (网卡显示名可能是中文)
 */
static void adapter_wide_to_utf8(const wchar_t* wide, char* out, const size_t out_len)
{
    if (out_len == 0) return;
    out[0] = '\0';
    if (wide == NULL || wide[0] == L'\0') return;
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, out, (int)out_len, NULL, NULL);
    out[out_len - 1] = '\0';
}

uint8_t get_adapters_stats(adapter_stats_t* out, const uint8_t max, char* default_id, const size_t default_id_len)
{
    if (out == NULL || max == 0) return 0;
    if (default_id != NULL && default_id_len > 0) default_id[0] = '\0';

    ULONG size = 16 * 1024;
    PIP_ADAPTER_ADDRESSES addrs = malloc(size);
    if (addrs == NULL) return 0;

    // INCLUDE_GATEWAYS: 靠"有没有网关"把 VMware/Hyper-V 这类虚拟网卡挡掉
    const ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER |
                        GAA_FLAG_INCLUDE_GATEWAYS;
    ULONG rc = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, addrs, &size);
    if (rc == ERROR_BUFFER_OVERFLOW)
    {
        free(addrs);
        addrs = malloc(size);
        if (addrs == NULL) return 0;
        rc = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, addrs, &size);
    }
    if (rc != NO_ERROR)
    {
        free(addrs);
        LOG_WARN("枚举网卡失败 (错误码 %lu)", (unsigned long)rc);
        return 0;
    }

    // 默认出口: 拿一个公网地址问系统走哪块网卡 (取不到就留空, 界面会退化成选第一块)
    DWORD best_index = 0;
    if (GetBestInterface(htonl(0x08080808), &best_index) != NO_ERROR) best_index = 0;

    uint8_t count = 0;
    for (PIP_ADAPTER_ADDRESSES addr = addrs; addr != NULL && count < max; addr = addr->Next)
    {
        // 只要真实联网网卡: 有线 / Wi-Fi / 蜂窝; 回环、隧道、桥接之类直接跳过
        if (addr->IfType != IF_TYPE_ETHERNET_CSMACD && addr->IfType != IF_TYPE_IEEE80211 &&
            addr->IfType != IF_TYPE_WWANPP && addr->IfType != IF_TYPE_WWANPP2)
        {
            continue;
        }

        MIB_IF_ROW2 row;
        memset(&row, 0, sizeof(row));
        row.InterfaceIndex = addr->IfIndex;
        if (GetIfEntry2(&row) != NO_ERROR) continue;
        // HardwareInterface 为假的是软件网卡 (VMware / Hyper-V / 蓝牙个人区域网)
        if (row.InterfaceAndOperStatusFlags.HardwareInterface == 0) continue;

        adapter_stats_t* item = &out[count];
        memset(item, 0, sizeof(*item));
        adapter_id_from_guid(addr->AdapterName, item->id, sizeof(item->id));
        // 只取 FriendlyName: 用户看到的就是系统设置里那个名字 (Description 各家 SDK 宽窄不一)
        adapter_wide_to_utf8(addr->FriendlyName, item->name, sizeof(item->name));
        // 未连接的网卡也列出来 (界面上标"未连接"), 免得用户以为没识别到
        item->connected = (addr->OperStatus == IfOperStatusUp && row.MediaConnectState == MediaConnectStateConnected);
        item->rx_bytes = row.InOctets;
        item->tx_bytes = row.OutOctets;

        if (best_index != 0 && addr->IfIndex == best_index && default_id != NULL && default_id_len > 0)
        {
            snprintf(default_id, default_id_len, "%s", item->id);
        }
        count++;
    }

    free(addrs);
    return count;
}

#else

uint8_t get_adapters_stats(adapter_stats_t* out, const uint8_t max, char* default_id, const size_t default_id_len)
{
    // 其它平台待实现: Linux 读 /proc/net/dev, macOS 读 sysctl(NET_RT_IFLIST2)
    (void)out;
    (void)max;
    if (default_id != NULL && default_id_len > 0) default_id[0] = '\0';
    return 0;
}

#endif

