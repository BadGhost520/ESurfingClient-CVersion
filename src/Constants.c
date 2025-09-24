//
// Created by bad_g on 2025/9/22.
//
#include <stdlib.h>
#include <time.h>

const char* USER_AGENT = "CCTP/android64_vpn/2093";
const char* REQUEST_ACCEPT = "text/html,text/xml,application/xhtml+xml,application/x-javascript,*/*";
const char* CAPTIVE_URL = "http://connect.rom.miui.com/generate_204";
const char* PORTAL_END_TAG = "//config.campus.js.chinatelecom.com-->";
const char* PORTAL_START_TAG = "<!--//config.campus.js.chinatelecom.com";
const char* AUTH_KEY = "Eshore!@#";
const char* HOST_NAME;

char* random_string(int length);

void initConstants()
{
    HOST_NAME = random_string(10);
}

char* random_string(int length) {
    // 定义字符集：大写字母 + 小写字母 + 数字
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    const int charset_size = sizeof(charset) - 1; // 减1是因为不包含字符串结束符'\0'

    // 参数验证
    if (length <= 0) {
        return NULL;
    }

    // 分配内存：length个字符 + 1个结束符
    char* result = (char*)malloc((length + 1) * sizeof(char));
    if (result == NULL) {
        return NULL; // 内存分配失败
    }

    // 初始化随机数种子（如果还没有初始化）
    static int seed_initialized = 0;
    if (!seed_initialized) {
        srand((unsigned int)time(NULL));
        seed_initialized = 1;
    }

    // 生成随机字符串
    for (int i = 0; i < length; i++) {
        int random_index = rand() % charset_size;
        result[i] = charset[random_index];
    }

    // 添加字符串结束符
    result[length] = '\0';

    return result;
}