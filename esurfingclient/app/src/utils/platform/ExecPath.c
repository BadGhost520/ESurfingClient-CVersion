#include "utils/platform/PlatformUtils.h"

#include "utils/log/Logger.h"
#include "utils/text/TextUtils.h"

#include "PlatformInternal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32

bool build_windows_cmdline(const char* exec_path, char* const argv[], char* out, const size_t len)
{
    if (exec_path == NULL || out == NULL || len == 0) return false;

    int used = snprintf(out, len, "\"%s\"", exec_path);
    if (used <= 0 || (size_t)used >= len) return false;

    for (int i = 1; argv != NULL && argv[i] != NULL; i++)
    {
        const int written = snprintf(out + used, len - (size_t)used, " %s", argv[i]);
        if (written <= 0 || (size_t)written >= len - (size_t)used) return false;
        used += written;
    }

    return true;
}

#endif

bool get_exec_path(char* path_array)
{
    if (path_array == NULL) return false;

#ifdef _WIN32
    char path[MAX_PATH];
    const DWORD len_d = GetModuleFileNameA(NULL, path, MAX_PATH);
    if (len_d == 0 || len_d >= MAX_PATH) return false;
    const uint16_t len = snprintf(path_array, PATH_MAX, "%s", safe_str(path));
    if ((size_t)len >= PATH_MAX) return false;
    return true;
#elif __linux__
    char path[PATH_MAX];
    const ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len <= 0 || len >= (ssize_t)sizeof(path)) return false;
    path[len] = '\0';
    const uint16_t n = snprintf(path_array, PATH_MAX, "%s", path);
    if ((size_t)n >= PATH_MAX) return false;
    return true;
#elif defined(__APPLE__)
    char path[PATH_MAX];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) != 0) return false;
    char* resolved = realpath(path, NULL);
    if (!resolved) return false;
    const uint16_t n = snprintf(path_array, PATH_MAX, "%s", resolved);
    free(resolved);
    if ((size_t)n >= PATH_MAX) return false;
    return true;
#else
    (void)path_array;
    return false;
#endif
}

bool get_exec_dir(char* dir_array)
{
    char path[PATH_MAX];
    if (get_exec_path(path) == false) return false;

    char* last = strrchr(path, '/');
#ifdef _WIN32
    char* last_win = strrchr(path, '\\');
    if (last_win != NULL && (last == NULL || last_win > last)) last = last_win;
#endif
    if (last == NULL) return false;
    *last = '\0';

    const uint16_t len = snprintf(dir_array, PATH_MAX, "%s", path);
    if ((size_t)len >= PATH_MAX) return false;
    return true;
}
