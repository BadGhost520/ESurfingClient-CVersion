//
// Created by bad_g on 2025/9/14.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "headFiles/NetClient.h"
#include "headFiles/PlatformUtils.h"
#include "headFiles/Session.h"
#include "headFiles/States.h"
#include "headFiles/NetworkStatus.h"
#include "headFiles/Constants.h"

void initSession()
{
    NetResult* result = simple_post(ticketUrl, algoId);
    if (result && result->type == NET_RESULT_SUCCESS) {
        ByteArray zsm;
        zsm.data = (unsigned char*)result->data;
        zsm.length = strlen(result->data);

        // 打印zsm数据的详细信息
        printf("=== ZSM数据分析 ===\n");
        printf("数据长度: %zu 字节\n", zsm.length);

        if (zsm.length > 0) {
            // 打印原始字符串形式
            printf("原始字符串: '%s'\n", (char*)zsm.data);

            // 打印十六进制形式
            printf("十六进制数据: ");
            for (size_t i = 0; i < zsm.length; i++) {
                printf("%02X ", zsm.data[i]);
                if ((i + 1) % 16 == 0) printf("\n                ");
            }
            printf("\n");

            // 打印ASCII可见字符形式
            printf("ASCII可见字符: ");
            for (size_t i = 0; i < zsm.length; i++) {
                if (zsm.data[i] >= 32 && zsm.data[i] <= 126) {
                    printf("%c", zsm.data[i]);
                } else {
                    printf(".");
                }
            }
            printf("\n");

            // 如果数据长度足够，解析结构
            if (zsm.length >= 4) {
                printf("\n=== 数据结构解析 ===\n");
                printf("头部 (前3字节): ");
                for (int i = 0; i < 3; i++) {
                    printf("%02X ", zsm.data[i]);
                }
                printf("('%c%c%c')\n", zsm.data[0], zsm.data[1], zsm.data[2]);

                printf("密钥长度 (第4字节): %d\n", zsm.data[3]);

                if (zsm.length > 4 + zsm.data[3]) {
                    printf("密钥数据: ");
                    for (int i = 4; i < 4 + zsm.data[3]; i++) {
                        printf("%02X ", zsm.data[i]);
                    }
                    printf("\n");

                    size_t pos = 4 + zsm.data[3];
                    if (pos < zsm.length) {
                        printf("算法ID长度: %d\n", zsm.data[pos]);
                        pos++;
                        if (pos + zsm.data[pos-1] <= zsm.length) {
                            printf("算法ID: ");
                            for (size_t i = pos; i < pos + zsm.data[pos-1]; i++) {
                                printf("%c", zsm.data[i]);
                            }
                            printf("\n");
                        }
                    }
                }
            }
        } else {
            printf("数据为空\n");
        }
        printf("==================\n\n");

        SessionInitialize(&zsm);
    } else {
        printf("请求失败: 无法创建结果对象\n");
    }

    if (result) {
        free_net_result(result);
    }
}

void authorization(void)
{
    initConstants();
    refreshStates();
    initSession();

    if (!isInitialized())
    {
        printf("初始化 Session 失败，请重启应用或从 Release 重新获取应用\n");
        printf("Release地址: https://github.com/BadGhost520/ESurfingClient-CVersion/releases\n");
        isRunning = false;
        return;
    }

    printf("Client IP: %s\n", userIp);
    printf("AC IP: %s\n", acIp);

    // 测试createXMLPayload函数
    char* xml_payload = createXMLPayload();
    if (xml_payload) {
        printf("生成的XML Payload:\n");
        printf("%s\n", xml_payload);
        free(xml_payload);
    } else {
        printf("创建XML Payload失败!\n");
    }

    printf("ClientId: %s\n", clientId);
    printf("algoID: %s\n", algoId);
    printf("macAddress: %s\n", macAddress);
    printf("authUrl: %s\n", authUrl);
    printf("ticketUrl: %s\n", ticketUrl);


}

void NetWorkStatus(void)
{
    while (1)
    {
        ConnectivityStatus networkStatus = detectConfig();
        switch (networkStatus)
        {
        case CONNECTIVITY_SUCCESS:
            printf("网络已连接\n");
            break;
        case CONNECTIVITY_REQUIRE_AUTHORIZATION:
            printf("需要认证\n");
            authorization();
            break;
        case CONNECTIVITY_REQUEST_ERROR:
            printf("网络错误\n");
            sleepSeconds(5);
            break;
        default:
            printf("未知错误\n");
            return;
        }
        sleepSeconds(10);
    }
}