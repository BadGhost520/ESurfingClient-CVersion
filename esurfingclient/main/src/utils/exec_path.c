#include "utils/PlatformUtils.h"

#include "utils/platform_internal.h"
#include "utils/Logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
