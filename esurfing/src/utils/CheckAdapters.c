#include <string.h>
#include <stdio.h>

#ifdef _WIN32

#include <winsock2.h>
#include <iphlpapi.h>
#pragma comment(lib, "IPHLPAPI.lib")

#else

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <ifaddrs.h>

#endif

#include "../headFiles/utils/cJSON.h"
#include "../headFiles/States.h"

char* getAdapterJSON()
{
    cJSON* root = cJSON_CreateObject();
    cJSON* adapters = cJSON_CreateArray();
#ifdef _WIN32
    PIP_ADAPTER_INFO pAdapterInfo = NULL;
    ULONG ulOutBufLen = 0;
    if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW)
    {
        pAdapterInfo = (PIP_ADAPTER_INFO)malloc(ulOutBufLen);
        if (pAdapterInfo && GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == NO_ERROR)
        {
            PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
            while (pAdapter)
            {
                cJSON* adapter = cJSON_CreateObject();
                cJSON_AddStringToObject(adapter, "name", pAdapter->Description);
                cJSON_AddStringToObject(adapter, "ip", pAdapter->IpAddressList.IpAddress.String);
                cJSON_AddItemToArray(adapters, adapter);
                pAdapter = pAdapter->Next;
            }
        }
    }
    if (pAdapterInfo) free(pAdapterInfo);
#else
    struct ifaddrs *ifaddrs_ptr, *ifa;
    if (getifaddrs(&ifaddrs_ptr) == 0)
    {
        for (ifa = ifaddrs_ptr; ifa; ifa = ifa->ifa_next)
        {
            if (!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET) continue;
            if (strcmp(ifa->ifa_name, "lo") == 0) continue;
            char ip[INET_ADDRSTRLEN];
            struct sockaddr_in *addr = (struct sockaddr_in*)ifa->ifa_addr;
            if (inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip)))
            {
                cJSON* adapter = cJSON_CreateObject();
                cJSON_AddStringToObject(adapter, "name", ifa->ifa_name);
                cJSON_AddStringToObject(adapter, "ip", ip);
                cJSON_AddItemToArray(adapters, adapter);
            }
        }
        freeifaddrs(ifaddrs_ptr);
    }
#endif
    cJSON_AddItemToObject(root, "adapters", adapters);
    char* json = cJSON_Print(root);
    cJSON_Delete(root);
    return json;
}