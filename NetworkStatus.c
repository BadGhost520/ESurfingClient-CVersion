//
// Created by bad_g on 2025/9/14.
//
#define CURL_STATICLIB
#include "headFiles/NetworkStatus.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>

// 网络状态检查函数
// 返回值: 0-返回204, 1-返回304, 2-其他状态码
int checkNetworkStatus(void) {
    CURL *curl;
    CURLcode res;
    long response_code = 0;
    
    // 初始化curl
    curl = curl_easy_init();
    if (!curl) {
        return 2; // 初始化失败，返回2
    }
    
    // 设置URL
    curl_easy_setopt(curl, CURLOPT_URL, "http://connect.rom.miui.com/generate_204");
    
    // 设置超时时间（10秒）
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    
    // 禁用输出到stdout
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, NULL);
    
    // 执行请求
    res = curl_easy_perform(curl);
    
    if (res == CURLE_OK) {
        // 获取HTTP响应码
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        
        // 清理curl
        curl_easy_cleanup(curl);
        
        // 根据响应码返回相应值
        if (response_code == 204) {
            return 0;
        }
        if (response_code == 302) {
            return 1;
        }
        return 2;
    } else {
        // 请求失败
        curl_easy_cleanup(curl);
        return 2;
    }
}