#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#ifdef _WIN32

#include <windows.h>
#include <io.h>

#else

#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#endif

#include "../headFiles/utils/PlatformUtils.h"
#include "../headFiles/utils/Logger.h"

#ifdef _WIN32
    static const char sep = '\\';
#else
    static const char sep = '/';
#endif

static LoggerSettings g_logger_settings = {0};
static const char fileName[] = "run.log";
static const char rotateFileName[] = ".rotate.log";

static LoggerConfig gLoggerConfig = {
    .level = LOG_LEVEL_INFO,
    .log_dir = "",
    .log_file = "",
    .file_handle = NULL,
    .max_lines = 10000,
    .current_lines = 0
};

static char* loggerLevelString(const LogLevel level)
{
    switch (level)
    {
        case LOG_LEVEL_DEBUG: return "调试";
        case LOG_LEVEL_INFO:  return "信息";
        case LOG_LEVEL_WARN:  return "警告";
        case LOG_LEVEL_ERROR: return "错误";
        case LOG_LEVEL_FATAL: return "致命";
        default:              return "未知";
    }
}

static void loggerRotateFile()
{
    if (!gLoggerConfig.file_handle || strlen(gLoggerConfig.log_file) == 0 || gLoggerConfig.current_lines < gLoggerConfig.max_lines) return;
    fclose(gLoggerConfig.file_handle);
    gLoggerConfig.file_handle = NULL;
    char* timeStr = getTime(FILE_FORMAT);
    if (!timeStr)
    {
        fprintf(stderr, "错误: 无法获取文件轮转时间\n");
        gLoggerConfig.file_handle = fopen(gLoggerConfig.log_file, "a");
        return;
    }
    char rotatedFilename[PATH_MAX];
    const int result = snprintf(rotatedFilename, sizeof(rotatedFilename), "%s%c%s%s", gLoggerConfig.log_dir, sep, timeStr, rotateFileName);
    free(timeStr);
    if (result >= (int)sizeof(rotatedFilename))
    {
        fprintf(stderr, "错误: 轮转的文件名过长 (最大 %zu)\n", sizeof(rotatedFilename) - 1);
        gLoggerConfig.file_handle = fopen(gLoggerConfig.log_file, "a");
        return;
    }
    rename(gLoggerConfig.log_file, rotatedFilename);
    gLoggerConfig.current_lines = 0;
    gLoggerConfig.file_handle = fopen(gLoggerConfig.log_file, "a");
    if (gLoggerConfig.file_handle == NULL) fprintf(stderr, "错误: 无法在轮转后重新打开日志文件 %s\n", gLoggerConfig.log_file);
}

static int getExecutableDir(char* out)
{
#ifdef _WIN32
    char path[MAX_PATH];
    const DWORD len = GetModuleFileNameA(NULL, path, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) return -1;
    char* last = strrchr(path, sep);
    if (!last) return -1;
    *last = '\0';
    const int n = snprintf(out, 260, "%s", path);
    if (n < 0 || (size_t)n >= 260) return -1;
    return 0;
#else
    char path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len <= 0 || len >= (ssize_t)sizeof(path)) return -1;
    path[len] = '\0';
    char* last = strrchr(path, sep);
    if (!last) return -1;
    *last = '\0';
    int n = snprintf(out, 260, "%s", path);
    if (n < 0 || (size_t)n >= 260) return -1;
    return 0;
#endif
}

static int ensureLogDir(char* out)
{
#ifdef _WIN32
    char dir[PATH_MAX];
    if (getExecutableDir(dir) != 0) return -1;
    const int n = snprintf(out, 260, "%s%clogs", dir, sep);
    if (n < 0 || (size_t)n >= 260) return -1;
    if (!CreateDirectoryA(out, NULL))
    {
        const DWORD err = GetLastError();
        if (err != ERROR_ALREADY_EXISTS) return -1;
    }
#else
    const char* dir = "/var/log/esurfing";
    if (isDebug && !isSmallDevice && access("/etc/openwrt_release", F_OK) == 0) dir = "/usr/esurfing";
    int n = snprintf(out, 260, "%s%clogs", dir, sep);
    if (n < 0 || (size_t)n >= 260) return -1;
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

static void loggerWriteToConsole(const char* message)
{
    printf("%s", message);
    fflush(stdout);
}

static void loggerWriteToFile(const char* message)
{
    if (gLoggerConfig.file_handle)
    {
        fprintf(gLoggerConfig.file_handle, "%s", message);
        fflush(gLoggerConfig.file_handle);
    }
}

void loggerLog(const LogLevel level, const char* file, const int line, const char* format, ...)
{
    if (level < gLoggerConfig.level) return;
    va_list args;
    char message[2048];
    char finalMessage[2560];
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    char* timestamp = getTime(CONSOLE_FORMAT);
    snprintf(finalMessage, sizeof(finalMessage),
        "[%s] [%s] [%s:%d] %s\n",
        timestamp,
        loggerLevelString(level),
        strrchr(file, sep) ? strrchr(file, sep) + 1 : file,
        line,
        message);
    loggerWriteToConsole(finalMessage);
    loggerWriteToFile(finalMessage);
    if (gLoggerConfig.file_handle) gLoggerConfig.current_lines++;
    loggerRotateFile();
    free(timestamp);
}

LoggerInitStatus loggerInit(const LoggerSettings logger_settings)
{
    g_logger_settings.is_debug = logger_settings.is_debug;
    if (g_logger_settings.is_debug) gLoggerConfig.level = LOG_LEVEL_DEBUG;
    else gLoggerConfig.level = LOG_LEVEL_INFO;
    g_logger_settings.is_small_device = logger_settings.is_small_device;
    if (ensureLogDir(gLoggerConfig.log_dir) != 0)
    {
        fprintf(stderr, "错误: 无法准备日志目录\n");
        return INIT_LOGGER_FAILURE;
    }
    const int ln = snprintf(gLoggerConfig.log_file, sizeof(gLoggerConfig.log_file), "%s%c%s", gLoggerConfig.log_dir, sep, fileName);
    if (ln < 0 || (size_t)ln >= sizeof(gLoggerConfig.log_file))
    {
        fprintf(stderr, "错误: 日志文件路径太长 (最大 %zu)\n", sizeof(gLoggerConfig.log_file));
        return INIT_LOGGER_FAILURE;
    }
    gLoggerConfig.file_handle = fopen(gLoggerConfig.log_file, "a");
    if (!gLoggerConfig.file_handle)
    {
        fprintf(stderr, "错误: 无法打开日志文件 %s\n", gLoggerConfig.log_file);
        return INIT_LOGGER_FAILURE;
    }
    LOG_DEBUG("日志等级: %s", loggerLevelString(gLoggerConfig.level));
    if (g_logger_settings.is_small_device && access("/etc/openwrt_release", F_OK) == 0) LOG_DEBUG("检测到 OpenWrt 环境，小容量设备模式已开启");
    return INIT_LOGGER_SUCCESS;
}

void loggerCleanup()
{
    LOG_DEBUG("正在关闭日志系统");
    if (!gLoggerConfig.file_handle)
    {
        LOG_WARN("日志系统未启动");
        return;
    }
    fclose(gLoggerConfig.file_handle);
    gLoggerConfig.file_handle = NULL;
    if (strlen(gLoggerConfig.log_file) == 0)
    {
        LOG_ERROR("日志路径为空");
        return;
    }
    char* timeStr = getTime(FILE_FORMAT);
    if (!timeStr)
    {
        fprintf(stderr, "错误: 无法获取文件清理时间\n");
        return;
    }
    char newFilename[PATH_MAX];
    const int result = snprintf(newFilename, sizeof(newFilename), "%s%c%s.log", gLoggerConfig.log_dir, sep, timeStr);
    free(timeStr);
    if (result >= (int)sizeof(newFilename))
    {
        fprintf(stderr, "错误: 新文件名过长 (最大 %zu)\n", sizeof(newFilename) - 1);
        return;
    }
    rename(gLoggerConfig.log_file, newFilename);
}

LogContent getLog()
{
    LogContent result = {NULL, 0};
    FILE* file = fopen(gLoggerConfig.log_file, "rb");
    if (!file)
    {
        perror("打开文件失败");
        return result;
    }
    fseek(file, 0, SEEK_END);
    const long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (file_size < 0)
    {
        fclose(file);
        return result;
    }
    result.data = (char*)malloc(file_size + 1);
    if (!result.data)
    {
        fclose(file);
        return result;
    }
    const size_t bytes_read = fread(result.data, 1, file_size, file);
    if (bytes_read != (size_t)file_size)
    {
        free(result.data);
        result.data = NULL;
    }
    else
    {
        result.data[file_size] = '\0';
        result.size = file_size;
    }
    fclose(file);
    return result;
}