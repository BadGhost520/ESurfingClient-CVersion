#include "utils/PlatformUtils.h"
#include "utils/Logger.h"

#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>

#ifdef _WIN32
#include <windows.h>

static const char sep = '\\';
#else
static const char sep = '/';
#endif

static const char s_file_name[] = "run.log";
static const char s_rotate_file_name[] = ".rotate.log";

static log_cfg_t s_logger_cfg = {
    .lv = LOG_LEVEL_INFO,
    .log_dir = "",
    .log_file = "",
    .file_handle = NULL,
    .max_lines = 10000,
    .cur_lines = 0
};

static const char* get_level_str(const LogLevel lv)
{
    switch (lv)
    {
    case LOG_LEVEL_VERBOSE: return "VERBOSE";
    case LOG_LEVEL_DEBUG:   return "DEBUG";
    case LOG_LEVEL_INFO:    return "INFO";
    case LOG_LEVEL_WARN:    return "WARN";
    case LOG_LEVEL_ERROR:   return "ERROR";
    case LOG_LEVEL_FATAL:   return "FATAL";
    default:                return "UNKNOWN";
    }
}

static void rotate()
{
    if (!s_logger_cfg.file_handle || strlen(s_logger_cfg.log_file) == 0 || s_logger_cfg.cur_lines < s_logger_cfg.max_lines) return;
    fclose(s_logger_cfg.file_handle);
    s_logger_cfg.file_handle = NULL;
    char cur_tm[32];
    get_fmt_time(cur_tm, FILE_FORMAT);
    char rotate_file_name[PATH_MAX];
    const uint16_t result = snprintf(rotate_file_name, sizeof(rotate_file_name), "%s%c%s%s", safe_str(s_logger_cfg.log_dir), sep, safe_str(cur_tm), s_rotate_file_name);
    if (result >= (uint16_t)sizeof(rotate_file_name))
    {
        fprintf(stderr, "ERROR: 轮转的文件名过长 (最大 %zu)\n", sizeof(rotate_file_name) - 1);
        s_logger_cfg.file_handle = fopen(s_logger_cfg.log_file, "a");
        return;
    }
    rename(s_logger_cfg.log_file, rotate_file_name);
    s_logger_cfg.cur_lines = 0;
    s_logger_cfg.file_handle = fopen(s_logger_cfg.log_file, "a");
    if (s_logger_cfg.file_handle == NULL) fprintf(stderr, "ERROR: 无法在轮转后重新打开日志文件 %s\n", s_logger_cfg.log_file);
}

#ifdef _WIN32
static bool get_exec_dir(char* out)
{
    char path[MAX_PATH];
    const DWORD len_d = GetModuleFileNameA(NULL, path, MAX_PATH);
    if (len_d == 0 || len_d >= MAX_PATH) return false;
    char* last = strrchr(path, sep);
    if (!last) return false;
    *last = '\0';
    const uint16_t len = snprintf(out, PATH_MAX, "%s", safe_str(path));
    if (len < 0 || (size_t)len >= PATH_MAX) return false;
    return true;
}
#endif

static bool get_log_dir(char* out)
{
#ifdef _WIN32
    char dir[PATH_MAX];
    if (get_exec_dir(dir) == false) return false;
    const uint16_t len = snprintf(out, PATH_MAX, "%s%clogs", safe_str(dir), sep);
    if ((size_t)len >= PATH_MAX) return false;
    if (!CreateDirectoryA(out, NULL))
    {
        const DWORD err = GetLastError();
        if (err != ERROR_ALREADY_EXISTS) return false;
    }
#else
    char dir[PATH_MAX] = "/var/log/esurfing";
    const uint16_t len = snprintf(out, PATH_MAX, "%s%clogs", safe_str(dir), sep);
    if ((size_t)len >= PATH_MAX) return false;
    struct stat st;
    if (stat(out, &st) != 0)
    {
        if (mkdir(dir, 0755) != 0 && errno != EEXIST) return false;
        if (mkdir(out, 0755) != 0 && errno != EEXIST) return false;
    }
    else if (!S_ISDIR(st.st_mode)) return false;
#endif
    return true;
}

static void write_2_console(const char* msg)
{
    printf("%s", msg);
    fflush(stdout);
}

static void write_2_file(const char* msg)
{
    if (s_logger_cfg.file_handle)
    {
        fprintf(s_logger_cfg.file_handle, "%s", msg);
        fflush(s_logger_cfg.file_handle);
    }
}

static char* get_thread_str()
{
    for (uint8_t i = 0; i < g_prog_cnt; i++)
    {
        if (sim_thread_cur_id() == g_prog_status[i].thread_id)
        {
            static char str[4];
            snprintf(str, sizeof(str), "%" PRIu8, i);
            return str;
        }
    }
    return "Main";
}

void log_out(const LogLevel level, const char* file, const uint32_t line, const char* fmt, ...)
{
    if (level > s_logger_cfg.lv) return;
    if (!s_logger_cfg.file_handle)
    {
        fprintf(stderr, "ERROR: 日志系统未打开, 无法输出日志\n");
        return;
    }
    va_list local_args;
    char ts[32];
    char msg[2048];
    char final_msg[2560];
    get_fmt_time(ts, CONSOLE_FORMAT);
    va_start(local_args, fmt);
    vsnprintf(msg, sizeof(msg), fmt, local_args);
    va_end(local_args);
    snprintf(final_msg, sizeof(final_msg),
        "[%s] [TID %" PRIu64 "] [T-%s] [%s] [%s:%d] %s\n",
        safe_str(ts),
        sim_thread_cur_id(),
        get_thread_str(),
        get_level_str(level),
        strrchr(file, '/') ? strrchr(file, '/') + 1 : strrchr(file, '\\') ? strrchr(file, '\\') + 1 : file,
        line,
        safe_str(msg));
    write_2_console(final_msg);
    write_2_file(final_msg);
    s_logger_cfg.cur_lines++;
    rotate();
}

LogLevel get_logger_level()
{
    return s_logger_cfg.lv;
}

void set_logger_level(const LogLevel lv)
{
    s_logger_cfg.lv = lv;
    LOG_INFO("设置日志等级为 [%s]", get_level_str(lv));
}

bool init_logger()
{
    if (get_log_dir(s_logger_cfg.log_dir) == false)
    {
        fprintf(stderr, "ERROR: 无法准备日志目录\n");
        return false;
    }
    const uint16_t len = snprintf(s_logger_cfg.log_file, sizeof(s_logger_cfg.log_file), "%s%c%s", safe_str(s_logger_cfg.log_dir), sep, s_file_name);
    if ((size_t)len >= sizeof(s_logger_cfg.log_file))
    {
        fprintf(stderr, "ERROR: 日志文件路径太长 (最大 %zu)\n", sizeof(s_logger_cfg.log_file));
        return false;
    }
    s_logger_cfg.file_handle = fopen(s_logger_cfg.log_file, "a");
    if (!s_logger_cfg.file_handle)
    {
        fprintf(stderr, "ERROR: 无法打开日志文件 %s\n", s_logger_cfg.log_file);
        return false;
    }
    LOG_DEBUG("日志系统初始化完成");
    LOG_DEBUG("日志等级: %s", get_level_str(s_logger_cfg.lv));
    return true;
}

void clean_logger()
{
    LOG_DEBUG("关闭日志系统");
    if (!s_logger_cfg.file_handle)
    {
        fprintf(stderr, "ERROR: 日志系统未启动\n");
        return;
    }
    fclose(s_logger_cfg.file_handle);
    s_logger_cfg.file_handle = NULL;
    if (strlen(s_logger_cfg.log_file) == 0)
    {
        fprintf(stderr, "ERROR: 日志路径为空\n");
        return;
    }
    char cur_tm[32];
    get_fmt_time(cur_tm, FILE_FORMAT);
    char new_file_name[PATH_MAX];
    snprintf(new_file_name, sizeof(new_file_name), "%s%c%s.log", safe_str(s_logger_cfg.log_dir), sep, safe_str(cur_tm));
    rename(s_logger_cfg.log_file, new_file_name);
}
