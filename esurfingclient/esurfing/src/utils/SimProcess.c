#include "utils/SimProcess.h"

#include <process.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils/PlatformUtils.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

void restart_process() {
#ifdef _WIN32
    char path[MAX_PATH];
    GetModuleFileName(NULL, path, MAX_PATH);
    STARTUPINFO si = {0};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {0};
    CreateProcess(path, path, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    ExitProcess(0);
#else
    char *argv[] = { "/proc/self/exe", NULL };
    execv(argv[0], argv);
    perror("execv");
    exit(1);
#endif
}
