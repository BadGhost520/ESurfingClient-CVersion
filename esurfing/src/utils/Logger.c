#include "utils/PlatformUtils.h"
#include "utils/Logger.h"

#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>

static const char sep = '\\';
#else

static const char sep = '/';
#endif

static const char fileName[] = "run.log";
static const char rotateFileName[] = ".rotate.log";
static uint64_t update_time = 0;
static uint64_t last_get_time = 0;

static LoggerConfig gLoggerConfig = {
    .level = LOG_LEVEL_INFO,
    .log_dir = "",
    .log_file = "",
    .file_handle = NULL,
    .max_lines = 10000,
    .current_lines = 0
};

static const char* loggerLevelString(const LogLevel level)
{
    switch (level)
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

#ifdef _WIN32
static int getExecutableDir(char* out)
{
    char path[MAX_PATH];
    const DWORD len = GetModuleFileNameA(NULL, path, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) return -1;
    char* last = strrchr(path, sep);
    if (!last) return -1;
    *last = '\0';
    const int n = snprintf(out, 260, "%s", path);
    if (n < 0 || (size_t)n >= 260) return -1;
    return 0;
}
#endif

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
    char dir[PATH_MAX] = "/var/log/esurfing";
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
    if (level > gLoggerConfig.level) return;
    va_list local_args;
    char message[2048];
    char finalMessage[2560];
    update_time = currentTimeMillis();
    va_start(local_args, format);
    vsnprintf(message, sizeof(message), format, local_args);
    va_end(local_args);
    char* timestamp = getTime(CONSOLE_FORMAT);
    snprintf(finalMessage, sizeof(finalMessage),
        "[%s] [配置 %d] [%s] [%s:%d] %s\n",
        timestamp,
        prog_index,
        loggerLevelString(level),
        strrchr(file, '/') ? strrchr(file, '/') + 1 : file,
        line,
        message);
    loggerWriteToConsole(finalMessage);
    loggerWriteToFile(finalMessage);
    if (gLoggerConfig.file_handle) gLoggerConfig.current_lines++;
    loggerRotateFile();
    free(timestamp);
}

LogLevel getLoggerLevel()
{
    return gLoggerConfig.level;
}

LoggerInitStatus loggerInit(const LogLevel logger_level)
{
    gLoggerConfig.level = logger_level;
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

static int createAtomicCopy(const char* src, const char* dst)
{
    int src_fd = open(src, O_RDONLY);
    if (src_fd < 0) return -1;
    int dst_fd = open(dst, O_WRONLY | O_CREAT | O_EXCL, 0644);
    if (dst_fd < 0)
    {
        close(src_fd);
        return -1;
    }
    struct stat st;
    if (fstat(src_fd, &st) != 0 || st.st_size == 0)
    {
        close(src_fd);
        close(dst_fd);
        remove(dst);
        return -1;
    }
#ifdef __linux__
    off_t offset = 0;
    if (sendfile(dst_fd, src_fd, &offset, st.st_size) == st.st_size)
    {
        close(src_fd);
        close(dst_fd);
        return 0;
    }
#endif
    char buffer[8192];
    ssize_t bytes;
    while ((bytes = read(src_fd, buffer, sizeof(buffer))) > 0)
    {
        if (write(dst_fd, buffer, bytes) != bytes)
            break;
    }
    close(src_fd);
    close(dst_fd);
    if (bytes < 0)
    {
        remove(dst);
        return -1;
    }
    return 0;
}

LogContent getLog(const bool check)
{
    LogContent result = {NULL, 0, true};
    static char temp_path[512];
    static long temp_counter = 0;
    snprintf(temp_path, sizeof(temp_path),
             "%s.web_%ld_%ld.tmp",
             gLoggerConfig.log_file,
             (long)time(NULL),
             __sync_fetch_and_add(&temp_counter, 1));
    if (last_get_time == update_time && check)
    {
        result.is_new = false;
        return result;
    }
    last_get_time = update_time;
    createAtomicCopy(gLoggerConfig.log_file, temp_path);
    FILE* file = fopen(temp_path, "r");
    if (!file)
    {
        remove(temp_path);
        return result;
    }
    fseek(file, 0, SEEK_END);
    const long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (file_size <= 0 || file_size > 10 * 1024 * 1024)
    {
        fclose(file);
        remove(temp_path);
        return result;
    }
    result.data = (char*)malloc(file_size + 1);
    if (!result.data)
    {
        fclose(file);
        remove(temp_path);
        return result;
    }
    const size_t bytes_read = fread(result.data, 1, file_size, file);
    result.data[bytes_read] = '\0';
    result.size = bytes_read;
    fclose(file);
    remove(temp_path);
    return result;
}