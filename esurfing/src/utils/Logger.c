//
// Created by bad_g on 2025/9/28.
//
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <stdlib.h>

#include "../headFiles/utils/Logger.h"
#include "../headFiles/utils/PlatformUtils.h"

#ifdef _WIN32
    #include <windows.h>
    #include <io.h>
#else
    #include <unistd.h>
    #include <sys/stat.h>
#endif

LoggerConfig gLoggerConfig = {
    .level = LOG_LEVEL_INFO,
    .logFile = "",
    .fileHandle = NULL,
    .maxFileSize = 10 * 1024 * 1024,  // 10MB
    .maxBackupFiles = 5
};

/**
 * 获取日志级别字符串函数
 */
const char* loggerLevelString(const LogLevel level)
{
    switch (level)
    {
    case LOG_LEVEL_DEBUG: return "DEBUG";
    case LOG_LEVEL_INFO:  return "INFO";
    case LOG_LEVEL_WARN:  return "WARN";
    case LOG_LEVEL_ERROR: return "ERROR";
    case LOG_LEVEL_FATAL: return "FATAL";
    case LOG_LEVEL_OFF:   return "OFF";
    default:              return "UNKNOWN";
    }
}

/**
 * 获取文件大小函数
 */
long loggerGetFileSize(const char* filename)
{
    FILE* file = fopen(filename, "r");
    if (file == NULL)
    {
        return 0;
    }
    fseek(file, 0, SEEK_END);
    const long size = ftell(file);
    fclose(file);
    return size;
}

/**
 * 文件轮转函数
 */
void loggerRotateFile()
{
    if (gLoggerConfig.fileHandle == NULL)
    {
        return;
    }
    fclose(gLoggerConfig.fileHandle);
    gLoggerConfig.fileHandle = NULL;
    const size_t baseLen = strlen(gLoggerConfig.logFile);
    const size_t bufSize = baseLen + 32;
    char* oldName = malloc(bufSize);
    char* newName = malloc(bufSize);
    if (oldName == NULL || newName == NULL)
    {
        fprintf(stderr, "Error: Unable to allocate rotation buffers\n");
        free(oldName);
        free(newName);
        return;
    }
    int n = snprintf(oldName, bufSize, "%s.%d",
                     gLoggerConfig.logFile, gLoggerConfig.maxBackupFiles);
    if (n < 0 || (size_t)n >= bufSize)
    {
        fprintf(stderr, "Error: Rotation path too long\n");
        free(oldName);
        free(newName);
        return;
    }
    remove(oldName);
    for (int i = gLoggerConfig.maxBackupFiles - 1; i >= 1; i--)
    {
        n = snprintf(oldName, bufSize, "%s.%d", gLoggerConfig.logFile, i);
        if (n < 0 || (size_t)n >= bufSize)
        {
            fprintf(stderr, "Error: Rotation path too long\n");
            break;
        }
        n = snprintf(newName, bufSize, "%s.%d", gLoggerConfig.logFile, i + 1);
        if (n < 0 || (size_t)n >= bufSize)
        {
            fprintf(stderr, "Error: Rotation path too long\n");
            break;
        }
        rename(oldName, newName);
    }
    n = snprintf(newName, bufSize, "%s.1", gLoggerConfig.logFile);
    if (n < 0 || (size_t)n >= bufSize)
    {
        fprintf(stderr, "Error: Rotation path too long\n");
        free(oldName);
        free(newName);
        return;
    }
    rename(gLoggerConfig.logFile, newName);
    free(oldName);
    free(newName);
    gLoggerConfig.fileHandle = fopen(gLoggerConfig.logFile, "w");
    if (gLoggerConfig.fileHandle == NULL)
    {
        fprintf(stderr, "Error: Unable to reopen log file after file rotation\n");
    }
}

/**
 * 检查并执行文件轮转函数
 */
void loggerRotateIfNeeded()
{
    if (gLoggerConfig.fileHandle == NULL || strlen(gLoggerConfig.logFile) == 0)
    {
        return;
    }
    const long fileSize = loggerGetFileSize(gLoggerConfig.logFile);
    if (fileSize > (long)gLoggerConfig.maxFileSize)
    {
        loggerRotateFile();
    }
}

/**
 * 获取可执行文件所在目录函数
 */
int getExecutableDir(char* out)
{
#ifdef _WIN32
    char path[MAX_PATH];
    const DWORD len = GetModuleFileNameA(NULL, path, MAX_PATH);
    if (len == 0 || len >= MAX_PATH)
    {
        return -1;
    }
    char* last = strrchr(path, '\\');
    if (!last)
    {
        return -1;
    }
    *last = '\0';
    const int n = snprintf(out, 260, "%s", path);
    if (n < 0 || (size_t)n >= 260)
    {
        return -1;
    }
    return 0;
#else
    char path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len <= 0 || len >= (ssize_t)sizeof(path))
    {
        return -1;
    }
    path[len] = '\0';
    char* last = strrchr(path, '/');
    if (!last)
    {
        return -1;
    }
    *last = '\0';
    int n = snprintf(out, 260, "%s", path);
    if (n < 0 || (size_t)n >= 260)
    {
        return -1;
    }
    return 0;
#endif
}

/**
 * 检查 log 目录函数
 */
int ensureLogDir(char* out)
{
    char dir[PATH_MAX];
    if (getExecutableDir(dir) != 0)
    {
        return -1;
    }
#ifdef _WIN32
    const int n = snprintf(out, 260, "%s\\logs", dir);
    if (n < 0 || (size_t)n >= 260)
    {
        return -1;
    }
    if (!CreateDirectoryA(out, NULL))
    {
        const DWORD err = GetLastError();
        if (err != ERROR_ALREADY_EXISTS)
        {
            return -1;
        }
    }
#else
    int n = snprintf(out, 260, "%s/log", dir);
    if (n < 0 || (size_t)n >= 260)
    {
        return -1;
    }
    struct stat st;
    if (stat(out, &st) != 0)
    {
        if (mkdir(out, 0755) != 0 && errno != EEXIST)
        {
            return -1;
        }
    }
    else if (!S_ISDIR(st.st_mode))
    {
        return -1;
    }
#endif
    return 0;
}

/**
 * 初始化日志系统函数
 */
int loggerInit(const LogLevel level)
{
    gLoggerConfig.level = level;
    char logDir[PATH_MAX];
    if (ensureLogDir(logDir) != 0)
    {
        fprintf(stderr, "Error: Unable to prepare log directory\n");
        return -1;
    }
    const char* logFilename = "run.log";
#ifdef _WIN32
    const char sep = '\\';
#else
    const char sep = '/';
#endif
    const int ln = snprintf(gLoggerConfig.logFile, sizeof(gLoggerConfig.logFile), "%s%c%s", logDir, sep, logFilename);
    if (ln < 0 || (size_t)ln >= sizeof(gLoggerConfig.logFile))
    {
        fprintf(stderr, "Error: Log file path too long (max %zu)\n", sizeof(gLoggerConfig.logFile));
        return -1;
    }
    gLoggerConfig.fileHandle = fopen(gLoggerConfig.logFile, "a");
    if (gLoggerConfig.fileHandle == NULL)
    {
        fprintf(stderr, "Error: Unable to open log file %s\n", gLoggerConfig.logFile);
        return -1;
    }
    LOG_DEBUG("Log level: %s", loggerLevelString(level));
    return 0;
}

/**
 * 清理日志系统函数
 */
void loggerCleanup()
{
    if (gLoggerConfig.fileHandle != NULL)
    {
        LOG_DEBUG("Shut down the logging system");
        fclose(gLoggerConfig.fileHandle);
        gLoggerConfig.fileHandle = NULL;
        if (strlen(gLoggerConfig.logFile) > 0)
        {
            time_t now;
            time(&now);
            char newFilename[PATH_MAX];
            char logDir[PATH_MAX];
            strcpy(logDir, gLoggerConfig.logFile);
            char* lastSep = strrchr(logDir,
                #ifdef _WIN32
                    '\\'
                #else
                    '/'
                #endif
            );
            if (lastSep != NULL)
            {
                *lastSep = '\0';  // 截断到目录路径
            }
            #ifdef _WIN32
            struct tm tmInfo;
            if (localtime_s(&tmInfo, &now) == 0)
            {
                char timeName[64];
                strftime(timeName, sizeof(timeName), "%Y%m%d_%H%M%S.log", &tmInfo);
                snprintf(newFilename, sizeof(newFilename), "%s\\%s", logDir, timeName);
                rename(gLoggerConfig.logFile, newFilename);
            }
            #else
            struct tm* tmInfo = localtime(&now);
            if (tmInfo != NULL)
            {
                char timeName[64];
                strftime(timeName, sizeof(timeName), "%Y%m%d_%H%M%S.log", tmInfo);
                snprintf(newFilename, sizeof(newFilename), "%s/%s", logDir, timeName);
                rename(gLoggerConfig.logFile, newFilename);
            }
            #endif
        }
    }
}

/**
 * 设置文件轮转函数
 */
void loggerSetRotation(const size_t maxSize, const int maxBackups)
{
    gLoggerConfig.maxFileSize = maxSize;
    gLoggerConfig.maxBackupFiles = maxBackups;
    LOG_INFO("File rotation setting: maximum %zu bytes, %d backup files", maxSize, maxBackups);
}

/**
 * 格式化时间戳函数
 */
void loggerFormatTimestamp(char* buffer)
{
    time_t now;
    time(&now);
    const struct tm* tmInfo = localtime(&now);
    strftime(buffer, 64, "%Y-%m-%d %H:%M:%S", tmInfo);
}

/**
 * 输出到控制台函数
 */
void loggerWriteToConsole(const char* message)
{
#ifdef _WIN32
    printf("%s", message);
#else
    printf("%s", message);
#endif
    fflush(stdout);
}

/**
 * 输出到文件函数
 */
void loggerWriteToFile(const char* message)
{
    if (gLoggerConfig.fileHandle != NULL)
    {
        fprintf(gLoggerConfig.fileHandle, "%s", message);
        fflush(gLoggerConfig.fileHandle);
    }
}

/**
 * 核心日志输出函数
 */
void loggerLog(const LogLevel level, const char* file, const int line, const char* format, ...)
{
    if (level < gLoggerConfig.level)
    {
        return;
    }
    va_list args;
    char message[2048];
    char finalMessage[2560];
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    char* timestamp = getTime();
    snprintf(finalMessage, sizeof(finalMessage),
        "[%s] [%s] [%s:%d] %s\n",
        timestamp,
        loggerLevelString(level),
        strrchr(file, '/') ? strrchr(file, '/') + 1 :
        strrchr(file, '\\') ? strrchr(file, '\\') + 1 : file,
        line,
        message);
    loggerWriteToConsole(finalMessage);
    loggerWriteToFile(finalMessage);
    loggerRotateIfNeeded();
    free(timestamp);
}