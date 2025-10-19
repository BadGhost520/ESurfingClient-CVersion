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
    .maxBackupFiles = 5,
    .maxLines = 100000,
    .currentLines = 0
};

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

void loggerRotateFile()
{
    if (gLoggerConfig.fileHandle == NULL || strlen(gLoggerConfig.logFile) == 0 || gLoggerConfig.currentLines < gLoggerConfig.maxLines)
    {
        return;
    }
    fclose(gLoggerConfig.fileHandle);
    gLoggerConfig.fileHandle = NULL;
    char rotatedFilename[PATH_MAX];
    char logDir[PATH_MAX];
    strcpy(logDir, gLoggerConfig.logFile);
    char* last_sep = strrchr(logDir,
        #ifdef _WIN32
            '\\'
        #else
            '/'
        #endif
    );
    if (last_sep != NULL)
    {
        *last_sep = '\0';
    }
    snprintf(rotatedFilename, sizeof(rotatedFilename), "%s\\%s.log", logDir, getFileTime());
    rename(gLoggerConfig.logFile, rotatedFilename);
    gLoggerConfig.currentLines = 0;
    gLoggerConfig.fileHandle = fopen(gLoggerConfig.logFile, "a");
    if (gLoggerConfig.fileHandle == NULL)
    {
        fprintf(stderr, "Error: Unable to reopen log file %s after rotation\n", gLoggerConfig.logFile);
    }
}

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

int loggerInit(const LogLevel level)
{
    gLoggerConfig.level = level;
    char logDir[PATH_MAX];
    if (ensureLogDir(logDir) != 0)
    {
        fprintf(stderr, "Error: Unable to prepare log directory\n");
        return -1;
    }
#ifdef _WIN32
    const char sep = '\\';
#else
    const char sep = '/';
#endif
    const int ln = snprintf(gLoggerConfig.logFile, sizeof(gLoggerConfig.logFile), "%s%crun.log", logDir, sep);
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

void loggerCleanup()
{
    if (gLoggerConfig.fileHandle != NULL)
    {
        LOG_DEBUG("Shut down the logging system");
        fclose(gLoggerConfig.fileHandle);
        gLoggerConfig.fileHandle = NULL;
        if (strlen(gLoggerConfig.logFile) > 0)
        {
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
                *lastSep = '\0';
            }
            snprintf(newFilename, sizeof(newFilename), "%s\\%s.log", logDir, getFileTime());
            rename(gLoggerConfig.logFile, newFilename);
        }
    }
}

void loggerWriteToConsole(const char* message)
{
#ifdef _WIN32
    printf("%s", message);
#else
    printf("%s", message);
#endif
    fflush(stdout);
}

void loggerWriteToFile(const char* message)
{
    if (gLoggerConfig.fileHandle != NULL)
    {
        fprintf(gLoggerConfig.fileHandle, "%s", message);
        fflush(gLoggerConfig.fileHandle);
    }
}

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
    if (gLoggerConfig.fileHandle != NULL)
    {
        gLoggerConfig.currentLines++;
    }
    loggerRotateFile();
    free(timestamp);
}