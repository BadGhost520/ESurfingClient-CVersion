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

static LoggerConfig s_logger_cfg = {
    .level = LOG_LEVEL_INFO,
    .log_dir = "",
    .log_file = "",
    .file_handle = NULL,
    .max_lines = 10000,
    .cur_lines = 0
};

static const char* get_level_str(const LogLevel level)
{
    switch (level)
    {
    case LOG_LEVEL_VERBOSE: return "VERB";
    case LOG_LEVEL_DEBUG:   return "DEBUG";
    case LOG_LEVEL_INFO:    return "INFO";
    case LOG_LEVEL_WARN:    return "WARN";
    case LOG_LEVEL_ERROR:   return "ERROR";
    case LOG_LEVEL_FATAL:   return "FATAL";
    default:                return "UNK";
    }
}

static void rotate()
{
    if (!s_logger_cfg.file_handle || strlen(s_logger_cfg.log_file) == 0 || s_logger_cfg.cur_lines < s_logger_cfg.max_lines) return;
    fclose(s_logger_cfg.file_handle);
    s_logger_cfg.file_handle = NULL;
    char cur_tm[32];
    get_fmt_time(cur_tm, FILE_FORMAT);
    char rotatedFilename[PATH_MAX];
    const uint16_t result = snprintf(rotatedFilename, sizeof(rotatedFilename), "%s%c%s%s", s_logger_cfg.log_dir, sep, cur_tm, s_rotate_file_name);
    if (result >= (uint16_t)sizeof(rotatedFilename))
    {
        fprintf(stderr, "错误: 轮转的文件名过长 (最大 %zu)\n", sizeof(rotatedFilename) - 1);
        s_logger_cfg.file_handle = fopen(s_logger_cfg.log_file, "a");
        return;
    }
    rename(s_logger_cfg.log_file, rotatedFilename);
    s_logger_cfg.cur_lines = 0;
    s_logger_cfg.file_handle = fopen(s_logger_cfg.log_file, "a");
    if (s_logger_cfg.file_handle == NULL) fprintf(stderr, "错误: 无法在轮转后重新打开日志文件 %s\n", s_logger_cfg.log_file);
}

#ifdef _WIN32
static uint8_t get_exec_dir(char* out)
{
    char path[MAX_PATH];
    const DWORD len = GetModuleFileNameA(NULL, path, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) return -1;
    char* last = strrchr(path, sep);
    if (!last) return -1;
    *last = '\0';
    const uint16_t n = snprintf(out, PATH_MAX, "%s", path);
    if (n < 0 || (size_t)n >= PATH_MAX) return -1;
    return 0;
}
#endif

static uint8_t ensure_log_dir(char* out)
{
#ifdef _WIN32
    char dir[PATH_MAX];
    if (get_exec_dir(dir) != 0) return -1;
    const uint16_t n = snprintf(out, PATH_MAX, "%s%clogs", dir, sep);
    if (n < 0 || (size_t)n >= PATH_MAX) return -1;
    if (!CreateDirectoryA(out, NULL))
    {
        const DWORD err = GetLastError();
        if (err != ERROR_ALREADY_EXISTS) return -1;
    }
#else
    char dir[PATH_MAX] = "/var/log/esurfing";
    const uint16_t n = snprintf(out, PATH_MAX, "%s%clogs", dir, sep);
    if (n < 0 || (size_t)n >= PATH_MAX) return -1;
    struct stat st;
    if (stat(out, &st) != 0)
    {
        if (mkdir(dir, 0755) != 0 && errno != EEXIST) return -1;
        if (mkdir(out, 0755) != 0 && errno != EEXIST) return -1;
    }
    else if (!S_ISDIR(st.st_mode)) return -1;
#endif
    return 0;
}

static void write_2_console(const char* message)
{
    printf("%s", message);
    fflush(stdout);
}

static void write_2_file(const char* message)
{
    if (s_logger_cfg.file_handle)
    {
        fprintf(s_logger_cfg.file_handle, "%s", message);
        fflush(s_logger_cfg.file_handle);
    }
}

void log_out(const LogLevel level, const char* file, const uint32_t line, const char* fmt, ...)
{
    if (level > s_logger_cfg.level) return;
    va_list local_args;
    char ts[32];
    char msg[2048];
    char final_msg[2560];
    get_fmt_time(ts, CONSOLE_FORMAT);
    va_start(local_args, fmt);
    vsnprintf(msg, sizeof(msg), fmt, local_args);
    va_end(local_args);
    snprintf(final_msg, sizeof(final_msg),
        "[%s] [%s] [%s:%d] %s\n",
        ts,
        get_level_str(level),
        strrchr(file, '/') ? strrchr(file, '/') + 1 : file,
        line,
        msg);
    write_2_console(final_msg);
    write_2_file(final_msg);
    if (s_logger_cfg.file_handle) s_logger_cfg.cur_lines++;
    rotate();
}

LogLevel get_logger_level()
{
    return s_logger_cfg.level;
}

LoggerInitStatus init_logger(const LogLevel logger_level)
{
    s_logger_cfg.level = logger_level;
    if (ensure_log_dir(s_logger_cfg.log_dir) != 0)
    {
        fprintf(stderr, "错误: 无法准备日志目录\n");
        return INIT_LOGGER_FAILURE;
    }
    const uint16_t ln = snprintf(s_logger_cfg.log_file, sizeof(s_logger_cfg.log_file), "%s%c%s", s_logger_cfg.log_dir, sep, s_file_name);
    if (ln < 0 || (size_t)ln >= sizeof(s_logger_cfg.log_file))
    {
        fprintf(stderr, "错误: 日志文件路径太长 (最大 %zu)\n", sizeof(s_logger_cfg.log_file));
        return INIT_LOGGER_FAILURE;
    }
    s_logger_cfg.file_handle = fopen(s_logger_cfg.log_file, "a");
    if (!s_logger_cfg.file_handle)
    {
        fprintf(stderr, "错误: 无法打开日志文件 %s\n", s_logger_cfg.log_file);
        return INIT_LOGGER_FAILURE;
    }
    LOG_DEBUG("日志系统初始化完成");
    LOG_DEBUG("日志等级: %s", get_level_str(s_logger_cfg.level));
    return INIT_LOGGER_SUCCESS;
}

void clean_logger()
{
    if (!s_logger_cfg.file_handle)
    {
        fprintf(stderr, "错误: 日志系统未启动\n");
        return;
    }
    LOG_DEBUG("关闭日志系统");
    fclose(s_logger_cfg.file_handle);
    s_logger_cfg.file_handle = NULL;
    if (strlen(s_logger_cfg.log_file) == 0)
    {
        fprintf(stderr, "错误: 日志路径为空\n");
        return;
    }
    char cur_tm[32];
    get_fmt_time(cur_tm, FILE_FORMAT);
    char new_file_name[PATH_MAX];
    snprintf(new_file_name, sizeof(new_file_name), "%s%c%s.log", s_logger_cfg.log_dir, sep, cur_tm);
    rename(s_logger_cfg.log_file, new_file_name);
}
