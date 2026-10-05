#include "utils/PlatformUtils.h"

#include "utils/Logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* extract_url_param(const char* url, const char* search_str_start)
{
    if (url == NULL || search_str_start == NULL)
    {
        LOG_ERROR("URL 为空");
        return NULL;
    }

    const size_t name_len = strlen(search_str_start);
    char search_pattern[64];
    if (name_len + 2 > sizeof(search_pattern))
    {
        LOG_ERROR("参数名过长");
        return NULL;
    }
    snprintf(search_pattern, sizeof(search_pattern), "%s=", search_str_start);

    const char* start = strstr(url, search_pattern);
    if (start == NULL)
    {
        LOG_ERROR("未找到参数: %s", search_pattern);
        return NULL;
    }
    start += name_len + 1;

    /* 最后一个参数后面没有 '&', 也要能截取到结尾或 '#' */
    const size_t value_len = strcspn(start, "&#");
    char* result = malloc(value_len + 1);
    if (result == NULL)
    {
        LOG_ERROR("分配内存失败");
        return NULL;
    }
    memcpy(result, start, value_len);
    result[value_len] = '\0';
    return result;
}

const char* safe_str(const char* str)
{
    return str ? str : "";
}
