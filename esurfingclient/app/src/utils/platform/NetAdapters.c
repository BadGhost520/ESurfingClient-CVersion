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

#elif defined(__linux__)

/**
 * @brief 从 sysfs 读一块网卡的累计收发字节数
 *
 * /sys/class/net/<网卡>/statistics 是内核给的 64 位计数器, OpenWrt (busybox/musl)
 * 上也一定有, 不用起进程、不用解析 /proc/net/dev 的对齐
 */
static bool adapter_read_counters(const char* name, uint64_t* rx_bytes, uint64_t* tx_bytes)
{
    char path[PATH_MAX];
    unsigned long long value;

    snprintf(path, sizeof(path), "/sys/class/net/%s/statistics/rx_bytes", name);
    FILE* file = fopen(path, "r");
    if (file == NULL) return false;
    const bool rx_ok = fscanf(file, "%llu", &value) == 1;
    if (rx_ok) *rx_bytes = (uint64_t)value;
    fclose(file);
    if (rx_ok == false) return false;

    snprintf(path, sizeof(path), "/sys/class/net/%s/statistics/tx_bytes", name);
    file = fopen(path, "r");
    if (file == NULL) return false;
    const bool tx_ok = fscanf(file, "%llu", &value) == 1;
    if (tx_ok) *tx_bytes = (uint64_t)value;
    fclose(file);
    return tx_ok;
}

/**
 * @brief 是不是回环口 (按接口类型判断, 名字不一定叫 lo)
 *
 * /sys/class/net/<网卡>/type 是 ARPHRD_*: 772 是回环, 1 是以太网
 */
static bool adapter_is_loopback(const char* name)
{
    char path[PATH_MAX];
    char type[16] = {0};

    snprintf(path, sizeof(path), "/sys/class/net/%s/type", name);
    FILE* file = fopen(path, "r");
    if (file == NULL) return false;
    const bool ok = fgets(type, sizeof(type), file) != NULL;
    fclose(file);
    if (ok == false) return false;
    return strtoul(type, NULL, 10) == 772; // ARPHRD_LOOPBACK
}

/**
 * @brief 网卡当前是否"通着"
 *
 * operstate 为 up 是正常情况; PPP/PPPoE 这类不报载波的接口显示 unknown,
 * 但它们确实在传数据, 不能算未连接
 */
static bool adapter_is_up(const char* name)
{
    char path[PATH_MAX];
    char state[16] = {0};

    snprintf(path, sizeof(path), "/sys/class/net/%s/operstate", name);
    FILE* file = fopen(path, "r");
    if (file == NULL) return false;
    const bool ok = fgets(state, sizeof(state), file) != NULL;
    fclose(file);
    if (ok == false) return false;
    return strncmp(state, "up", 2) == 0 || strncmp(state, "unknown", 7) == 0;
}

/**
 * @brief 找默认出口接口 (/proc/net/route 里目标与掩码都为 0 的那条, 取 metric 最小的)
 *
 * 路由表里目标是 0.0.0.0/0 的就是默认路由; 没有 (比如纯 IPv6 上联) 时留空,
 * 网页会退化成选第一块网卡
 */
static void adapter_default_route(char* out, const size_t out_len)
{
    if (out == NULL || out_len == 0) return;
    out[0] = '\0';

    FILE* file = fopen("/proc/net/route", "r");
    if (file == NULL) return;

    char line[256];
    if (fgets(line, sizeof(line), file) == NULL)
    {
        fclose(file);
        return;
    }

    bool found = false;
    unsigned long best_metric = 0;
    while (fgets(line, sizeof(line), file) != NULL)
    {
        char iface[64] = {0};
        unsigned long dest = 0, gateway = 0, flags = 0, metric = 0, mask = 0;
        // Iface Destination Gateway Flags RefCnt Use Metric Mask ...
        if (sscanf(line, "%63s %lx %lx %lx %*u %*u %lu %lx", iface, &dest, &gateway, &flags, &metric, &mask) != 6)
        {
            continue;
        }
        if (dest != 0 || mask != 0) continue;
        if (found == false || metric < best_metric)
        {
            snprintf(out, out_len, "%s", iface);
            best_metric = metric;
            found = true;
        }
    }
    fclose(file);
}

uint8_t get_adapters_stats(adapter_stats_t* out, const uint8_t max, char* default_id, const size_t default_id_len)
{
    if (out == NULL || max == 0) return 0;
    if (default_id != NULL && default_id_len > 0) adapter_default_route(default_id, default_id_len);

    struct ifaddrs* list = NULL;
    if (getifaddrs(&list) != 0) return 0;

    uint8_t count = 0;
    for (struct ifaddrs* ifa = list; ifa != NULL && count < max; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL) continue;
        if (ifa->ifa_addr->sa_family != AF_INET && ifa->ifa_addr->sa_family != AF_INET6) continue;
        if (adapter_is_loopback(ifa->ifa_name)) continue;

        // 同一块网卡会有 IPv4/IPv6 多条记录, 只取第一次见到的那条
        bool seen = false;
        for (uint8_t i = 0; i < count; i++)
        {
            if (strcmp(out[i].id, ifa->ifa_name) == 0)
            {
                seen = true;
                break;
            }
        }
        if (seen) continue;

        uint64_t rx_bytes = 0, tx_bytes = 0;
        if (adapter_read_counters(ifa->ifa_name, &rx_bytes, &tx_bytes) == false) continue;

        adapter_stats_t* item = &out[count];
        memset(item, 0, sizeof(*item));
        snprintf(item->id, sizeof(item->id), "%s", ifa->ifa_name);
        snprintf(item->name, sizeof(item->name), "%s", ifa->ifa_name);
        item->connected = adapter_is_up(ifa->ifa_name);
        item->rx_bytes = rx_bytes;
        item->tx_bytes = tx_bytes;
        count++;
    }

    freeifaddrs(list);
    return count;
}

#else

uint8_t get_adapters_stats(adapter_stats_t* out, const uint8_t max, char* default_id, const size_t default_id_len)
{
    // macOS 等平台待实现 (macOS 读 sysctl(NET_RT_IFLIST2))
    (void)out;
    (void)max;
    if (default_id != NULL && default_id_len > 0) default_id[0] = '\0';
    return 0;
}

#endif

