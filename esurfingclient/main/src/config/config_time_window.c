#include "states/States.h"

#include "config/config_internal.h"

#include "utils/Logger.h"

#include "cJSON/cJSON.h"

#include <ctype.h>
#include <string.h>

/**
 * @brief 将英文星期缩写转为周起始偏移 (0=周日 ... 6=周六)
 * @return 0-6, 失败返回 -1
 */
static int week_day_from_str(const char* str)
{
    if (!str) return -1;
    const char d0 = (char)tolower((unsigned char)str[0]);
    const char d1 = (char)tolower((unsigned char)str[1]);
    const char d2 = (char)tolower((unsigned char)str[2]);

    if (d0 == 'm' && d1 == 'o' && d2 == 'n') return 1;
    if (d0 == 't' && d1 == 'u' && d2 == 'e') return 2;
    if (d0 == 'w' && d1 == 'e' && d2 == 'd') return 3;
    if (d0 == 't' && d1 == 'h' && d2 == 'u') return 4;
    if (d0 == 'f' && d1 == 'r' && d2 == 'i') return 5;
    if (d0 == 's' && d1 == 'a' && d2 == 't') return 6;
    if (d0 == 's' && d1 == 'u' && d2 == 'n') return 0;
    return -1;
}

/**
 * @brief 解析 "mon 08:13" 格式
 * @param str 原始字符串
 * @param week_min 输出周分钟 (0-10079)
 * @return 是否合法
 */
static bool parse_week_time(const char* str, uint16_t* week_min)
{
    if (!str || strlen(str) != 9) return false;
    if (str[3] != ' ') return false;
    if (isdigit((unsigned char)str[4]) == 0 ||
        isdigit((unsigned char)str[5]) == 0 ||
        isdigit((unsigned char)str[7]) == 0 ||
        isdigit((unsigned char)str[8]) == 0 ||
        str[6] != ':')
    {
        return false;
    }

    const int day = week_day_from_str(str);
    if (day < 0) return false;

    const int hour = (str[4] - '0') * 10 + (str[5] - '0');
    const int minute = (str[7] - '0') * 10 + (str[8] - '0');
    if (hour > 23 || minute > 59) return false;

    if (week_min) *week_min = (uint16_t)(day * 1440 + hour * 60 + minute);
    return true;
}

/**
 * @brief 解析一个 time_windows 数组元素 { "start": "mon 08:13", "end": "sun 23:57" }
 * @param item cJSON 对象
 * @param win 输出窗口
 * @return 是否合法 (end <= start 时按跨周处理)
 */
static bool parse_time_window(const cJSON* item, time_window_t* win)
{
    if (!item || !cJSON_IsObject(item)) return false;

    const cJSON* start_item = cJSON_GetObjectItem(item, "start");
    const cJSON* end_item = cJSON_GetObjectItem(item, "end");
    if (!start_item || !cJSON_IsString(start_item) ||
        !end_item || !cJSON_IsString(end_item))
    {
        return false;
    }

    uint16_t start = 0;
    uint16_t end = 0;
    if (parse_week_time(start_item->valuestring, &start) == false ||
        parse_week_time(end_item->valuestring, &end) == false)
    {
        return false;
    }

    if (start == end) return false;

    if (end <= start)
    {
        end = (uint16_t)(end + WEEK_MINUTES);
    }

    if (win)
    {
        win->start_week_min = start;
        win->end_week_min = end;
    }
    return true;
}

/**
 * @brief 从 cJSON 数组解析 time_windows
 * @param arr cJSON 数组 (允许 NULL/空)
 * @param windows 输出窗口数组
 * @param count 输出窗口数量
 * @return 是否合法
 */
static bool parse_time_windows(const cJSON* arr, time_window_t* windows, uint8_t* count)
{
    if (count) *count = 0;

    if (arr == NULL)
    {
        return true;
    }
    if (cJSON_IsArray(arr) == false)
    {
        return false;
    }

    const int size = cJSON_GetArraySize(arr);
    if (size < 0 || size > MAX_TIME_WINDOWS) return false;

    for (int i = 0; i < size; i++)
    {
        time_window_t win;
        if (parse_time_window(cJSON_GetArrayItem(arr, i), &win) == false)
        {
            return false;
        }
        if (windows) windows[i] = win;
    }

    if (count) *count = (uint8_t)size;
    return true;
}

/**
 * @brief 解析 time_windows 并写入 login_cfg
 * @return 是否合法
 */
bool apply_time_windows(const cJSON* item, login_cfg_t* cfg)
{
    if (parse_time_windows(item, cfg->time_windows, &cfg->time_window_count) == false)
    {
        return false;
    }
    cfg->has_time_control = cfg->time_window_count > 0;
    return true;
}
