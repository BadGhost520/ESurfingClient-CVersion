#include "utils/PlatformUtils.h"

#include "states/States.h"

#include "utils/PlatformInternal.h"
#include "utils/Logger.h"

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
