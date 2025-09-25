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

void getTicket()
{
    char* payload = createXMLPayload();
    char* encry = encrypt(payload);
    printf("加密后数据:\n%s\n", encry);
    char* decry = decrypt(encry);
    printf("解密后数据:\n%s\n", decry);
    NetResult* result = simple_post(ticketUrl, encry);
    if (result && result->type == NET_RESULT_SUCCESS && result->data != NULL)
    {
        // 首先尝试解密响应数据
        char* decrypted_response = decrypt(result->data);
        if (decrypted_response) {
            printf("解密后的服务器响应:\n%s\n", decrypted_response);

            // 提取result字段
            char* result_value = extract_xml_tag_content(decrypted_response, "result");
            if (result_value && strlen(result_value) > 0) {
                printf("提取到result数据: %s\n", result_value);
                free(result_value);
            } else {
                printf("result 数据为空！\n");

                // 尝试提取ticket字段作为备选
                char* ticket_value = extract_xml_tag_content(decrypted_response, "ticket");
                if (ticket_value && strlen(ticket_value) > 0) {
                    printf("提取到ticket数据: %s\n", ticket_value);
                    free(ticket_value);
                } else {
                    printf("ticket 数据也为空！\n");
                }
            }

            free(decrypted_response);
        } else {
            printf("解密服务器响应失败！\n");

            // 如果解密失败，尝试直接解析原始响应
            printf("尝试直接解析原始响应数据...\n");
            char* result_value = extract_xml_tag_content(result->data, "result");
            if (result_value && strlen(result_value) > 0) {
                printf("从原始响应提取到result数据: %s\n", result_value);
                free(result_value);
            } else {
                printf("原始响应中也没有找到result数据！\n");
                printf("原始响应内容: %s\n", result->data);
            }
        }

        ByteArray zsm;
        zsm.data = (unsigned char*)result->data;
        zsm.length = strlen(result->data);
        // char* data = decrypt(result->data);
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

            // 检查数据是否为十六进制字符串格式并进行结构解析
            if (zsm.length >= 4) {
                printf("\n=== 数据结构解析 ===\n");

                // 检查是否为十六进制字符串格式
                int is_hex_string = 1;
                for (size_t i = 0; i < zsm.length && i < 10; i++) {
                    unsigned char c = zsm.data[i];
                    if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) {
                        is_hex_string = 0;
                        break;
                    }
                }

                unsigned char* binary_data = NULL;
                size_t binary_len = 0;
                const unsigned char* data_to_parse = NULL;

                if (is_hex_string) {
                    printf("检测到十六进制字符串格式，正在转换为二进制数据进行解析...\n");
                    if (hex_string_to_binary((const char*)zsm.data, zsm.length, &binary_data, &binary_len)) {
                        data_to_parse = binary_data;
                        printf("转换后的二进制数据长度: %zu\n", binary_len);
                    } else {
                        printf("错误: 十六进制字符串转换失败，使用原始数据解析\n");
                        data_to_parse = zsm.data;
                        binary_len = zsm.length;
                    }
                } else {
                    printf("检测到二进制数据格式，直接解析\n");
                    data_to_parse = zsm.data;
                    binary_len = zsm.length;
                }

                if (data_to_parse && binary_len >= 4) {
                    printf("头部 (前3字节): ");
                    for (int i = 0; i < 3; i++) {
                        printf("%02X ", data_to_parse[i]);
                    }
                    printf("('%c%c%c')\n", data_to_parse[0], data_to_parse[1], data_to_parse[2]);

                    printf("密钥长度 (第4字节): %d\n", data_to_parse[3]);

                    if (binary_len > 4 + data_to_parse[3]) {
                        printf("密钥数据: ");
                        for (int i = 4; i < 4 + data_to_parse[3]; i++) {
                            printf("%02X ", data_to_parse[i]);
                        }
                        printf("\n");

                        size_t pos = 4 + data_to_parse[3];
                        if (pos < binary_len) {
                            printf("算法ID长度: %d\n", data_to_parse[pos]);
                            pos++;
                            if (pos + data_to_parse[pos-1] <= binary_len) {
                                printf("算法ID: ");
                                for (size_t i = pos; i < pos + data_to_parse[pos-1]; i++) {
                                    printf("%c", data_to_parse[i]);
                                }
                                printf("\n");
                            }
                        }
                    }
                }

                // 释放分配的内存
                if (binary_data) {
                    free(binary_data);
                }
            }
            printf("data: %s\n", decrypt(result->data));
        } else {
            printf("数据为空\n");
        }
        printf("==================\n\n");
    } else {
        printf("result 数据为空！\n");
    }
    session_free();
    free(payload);
}

void initSession()
{
    NetResult* result = simple_post(ticketUrl, algoId);
    if (result && result->type == NET_RESULT_SUCCESS && result->data != NULL) {
        // 首先尝试解密响应数据
        char* decrypted_response = decrypt(result->data);
        if (decrypted_response) {
            printf("解密后的服务器响应:\n%s\n", decrypted_response);

            // 提取result字段
            char* result_value = extract_xml_tag_content(decrypted_response, "result");
            if (result_value && strlen(result_value) > 0) {
                printf("提取到result数据: %s\n", result_value);

                // 使用result数据初始化Session
                ByteArray zsm;
                zsm.data = (unsigned char*)result_value;
                zsm.length = strlen(result_value);
                initialize(&zsm);

                free(result_value);
            } else {
                printf("result 数据为空！\n");

                // 尝试提取其他可能的字段
                char* session_value = extract_xml_tag_content(decrypted_response, "session");
                if (session_value && strlen(session_value) > 0) {
                    printf("提取到session数据: %s\n", session_value);

                    ByteArray zsm;
                    zsm.data = (unsigned char*)session_value;
                    zsm.length = strlen(session_value);
                    initialize(&zsm);

                    free(session_value);
                } else {
                    printf("session 数据也为空！\n");

                    // 如果XML解析失败，尝试直接使用解密后的数据
                    printf("尝试直接使用解密后的数据初始化Session...\n");
                    ByteArray zsm;
                    zsm.data = (unsigned char*)decrypted_response;
                    zsm.length = strlen(decrypted_response);
                    initialize(&zsm);
                }
            }

            free(decrypted_response);
        } else {
            printf("解密服务器响应失败！\n");

            // 如果解密失败，尝试直接解析原始响应
            printf("尝试直接解析原始响应数据...\n");
            char* result_value = extract_xml_tag_content(result->data, "result");
            if (result_value && strlen(result_value) > 0) {
                printf("从原始响应提取到result数据: %s\n", result_value);

                ByteArray zsm;
                zsm.data = (unsigned char*)result_value;
                zsm.length = strlen(result_value);
                initialize(&zsm);

                free(result_value);
            } else {
                printf("原始响应中也没有找到result数据！\n");
                printf("原始响应内容: %s\n", result->data);

                // 最后尝试直接使用原始响应数据
                printf("尝试直接使用原始响应数据初始化Session...\n");
                ByteArray zsm;
                zsm.data = (unsigned char*)result->data;
                zsm.length = strlen(result->data);
                initialize(&zsm);
            }
        }
    } else {
        printf("获取Session数据失败\n");
        if (result) {
            if (result->type != NET_RESULT_SUCCESS) {
                printf("网络请求失败: %s\n", result->error_message ? result->error_message : "未知错误");
            } else if (result->data == NULL) {
                printf("服务器返回的数据为空\n");
            }
        } else {
            printf("网络请求返回NULL\n");
        }
    }

    free_net_result(result);
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

    getTicket();

    printf("ClientId: %s\n", clientId);
    printf("algoID: %s\n", algoId);
    printf("macAddress: %s\n", macAddress);
    printf("authUrl: %s\n", authUrl);
    printf("ticketUrl: %s\n", ticketUrl);


}

void NetWorkStatus()
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