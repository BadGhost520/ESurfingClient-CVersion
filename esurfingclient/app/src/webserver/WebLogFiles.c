#include "webserver/WebInternal.h"

#include <mongoose/mongoose.h>

#include "utils/PlatformUtils.h"
#include "utils/Logger.h"

static const char log_current_name[] = "run.log";

static const char log_suffix[] = ".log";

static const char* week_day_to_str(const int day)
{
    static const char* names[] = {"sun", "mon", "tue", "wed", "thu", "fri", "sat"};
    if (day < 0 || day > 6) return "sun";
    return names[day];
}

void format_week_min(const uint16_t week_min, char* out)
{
    const uint16_t mod = week_min % WEEK_MINUTES;
    const int day = mod / 1440;
    const int hour = (mod % 1440) / 60;
    const int minute = mod % 60;
    snprintf(out, TIME_WINDOW_STR_LEN, "%s %02d:%02d", week_day_to_str(day), hour, minute);
}

/**
 * @brief 检查日志文件名是否安全 (禁止路径穿越)
 * @param name 文件名
 * @return 是否安全
 */
bool is_safe_log_name(const char* name)
{
    if (name == NULL) return false;
    const size_t len = strlen(name);
    if (len == 0 || len >= LOG_NAME_LEN) return false;
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) return false;
    for (size_t i = 0; i < len; i++)
    {
        const unsigned char c = (unsigned char)name[i];
        if (isalnum(c) == 0 && c != '.' && c != '_' && c != '-') return false;
    }
    return true;
}

/**
 * @brief 检查文件名是不是日志文件
 *
 * 必须再认一次后缀: 日志目录默认就是【程序所在目录】(配置里的 log_dir),
 * 那里还躺着程序本体 / 配置文件 / portal, 只判"名字安全"的话
 * 日志页会把这些都列成日志, 连明文账号密码所在的 ESurfingClient.json
 * 都能当成日志读出来
 * @param name 文件名
 * @return 是否是日志文件
 */
bool is_log_file_name(const char* name)
{
    if (is_safe_log_name(name) == false) return false;

    const size_t name_len = strlen(name);
    const size_t suffix_len = sizeof(log_suffix) - 1;
    if (name_len <= suffix_len) return false;

    return str_case_cmp(name + name_len - suffix_len, log_suffix) == 0;
}

/**
 * @brief 读取日志目录中的文件列表
 * @param out 输出数组
 * @param max 数组容量
 * @return 文件数量
 */
int list_log_files(log_file_entry_t* out, const int max)
{
    const char* dir = get_logger_dir();
    if (dir == NULL || dir[0] == '\0') return 0;

    int count = 0;

#ifdef _WIN32

    char pattern[PATH_MAX];
    const int pattern_len = snprintf(pattern, sizeof(pattern), "%s%c*", dir, SEP);
    if (pattern_len <= 0 || (size_t)pattern_len >= sizeof(pattern)) return 0;

    WIN32_FIND_DATAA find_data;
    HANDLE handle = FindFirstFileA(pattern, &find_data);
    if (handle == INVALID_HANDLE_VALUE) return 0;
    do
    {
        if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) continue;
        if (count >= max) break;
        if (is_log_file_name(find_data.cFileName) == false) continue;

        snprintf(out[count].name, sizeof(out[count].name), "%s", find_data.cFileName);
        out[count].size = ((uint64_t)find_data.nFileSizeHigh << 32) | (uint64_t)find_data.nFileSizeLow;
        // FILETIME (100 纳秒, 1601 起) -> Unix 时间戳 (秒, 1970 起)
        const uint64_t file_time = ((uint64_t)find_data.ftLastWriteTime.dwHighDateTime << 32) |
            (uint64_t)find_data.ftLastWriteTime.dwLowDateTime;
        out[count].mtime = file_time / 10000000ULL - 11644473600ULL;
        out[count].current = strcmp(find_data.cFileName, log_current_name) == 0;

        char path[PATH_MAX];
        const int path_len = snprintf(path, sizeof(path), "%s%c%s", dir, SEP, find_data.cFileName);
        if (path_len > 0 && (size_t)path_len < sizeof(path))
        {
            win_stat_file(path, &out[count].size, &out[count].mtime);
        }
        count++;
    } while (FindNextFileA(handle, &find_data) != 0);
    FindClose(handle);

#else

    DIR* dir_handle = opendir(dir);
    if (dir_handle == NULL) return 0;

    char path[PATH_MAX];
    struct dirent* entry;
    while ((entry = readdir(dir_handle)) != NULL)
    {
        if (count >= max) break;
        if (is_log_file_name(entry->d_name) == false) continue;

        const int path_len = snprintf(path, sizeof(path), "%s%c%s", dir, SEP, entry->d_name);
        if (path_len <= 0 || (size_t)path_len >= sizeof(path)) continue;

        struct stat st;
        if (stat(path, &st) != 0 || !S_ISREG(st.st_mode)) continue;

        snprintf(out[count].name, sizeof(out[count].name), "%s", entry->d_name);
        out[count].size = (uint64_t)st.st_size;
        out[count].mtime = (uint64_t)st.st_mtime;
        out[count].current = strcmp(entry->d_name, log_current_name) == 0;
        count++;
    }
    closedir(dir_handle);

#endif

    return count;
}

/** @brief 排序: 当前日志在最前, 其余按修改时间倒序 */
int cmp_log_files(const void* a, const void* b)
{
    const log_file_entry_t* left = (const log_file_entry_t*)a;
    const log_file_entry_t* right = (const log_file_entry_t*)b;
    if (left->current != right->current) return left->current ? -1 : 1;
    if (left->mtime != right->mtime) return left->mtime > right->mtime ? -1 : 1;
    return -str_case_cmp(left->name, right->name);
}

void logFn(const char ch, void *param)
{
    (void)param;
    static char buffer[512];
    static size_t pos = 0;
    if (ch == '\n' || pos >= sizeof(buffer) - 1)
    {
        if (pos > 0)
        {
            const char* web_log_level = strchr(buffer, ' ');
            if (!web_log_level)
            {
                LOG_WARN("未知的 Web 日志: %s", buffer);
                return;
            }
            const char* file_start = web_log_level + 3;
            const char* file_end = strchr(file_start, ':');
            if (!file_end)
            {
                LOG_WARN("未知的 Web 日志: %s", buffer);
                return;
            }
            const size_t file_length = file_end - file_start;
            char* file = malloc(file_length + 1);
            if (!file)
            {
                LOG_WARN("分配内存失败");
                return;
            }
            memcpy(file, file_start, file_length);
            file[file_length] = '\0';
            const char* file_line_start = file_end + 1;
            const char* file_line_end = strchr(file_line_start, ':');
            if (!file_line_end)
            {
                LOG_WARN("未知的 Web 日志: %s", buffer);
                return;
            }
            const size_t file_line_length = file_line_end - file_line_start;
            char* file_line_str = malloc(file_line_length + 1);
            if (!file_line_str)
            {
                LOG_WARN("分配内存失败");
                return;
            }
            memcpy(file_line_str, file_line_start, file_line_length);
            file_line_str[file_line_length] = '\0';
            const uint64_t file_line = str2uint64(file_line_str);
            const char* msg = file_line_end + 1;
            switch(web_log_level[1])
            {
            case '1':
                LOG_WEB_ERROR(file, file_line, "%s", msg);
                break;
            case '2':
                LOG_WEB_INFO(file, file_line, "%s", msg);
                break;
            default:
                LOG_WEB_VERBOSE(file, file_line, "%s", msg);
            }
            free(file);
            free(file_line_str);
        }
        pos = 0;
    }
    else if (ch != '\r')
    {
        buffer[pos++] = ch;
    }
}
