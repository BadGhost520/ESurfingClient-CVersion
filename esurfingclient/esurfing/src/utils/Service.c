#include "utils/Shutdown.h"
#include "utils/Service.h"
#include "utils/Logger.h"

#include <setjmp.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

extern bool g_need_restart;
extern jmp_buf g_exit_jmp;

extern void work();

#ifdef _WIN32

static SERVICE_STATUS        g_ServiceStatus = {0};
static SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
static HANDLE                g_ServiceStopEvent = INVALID_HANDLE_VALUE;
static bool service_mode = false;

static void WINAPI ServiceCtrlHandler(DWORD CtrlCode)
{
    if (CtrlCode == SERVICE_CONTROL_STOP)
    {
        LOG_DEBUG("接收到 SCM 服务结束信号");
        g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
        SetEvent(g_ServiceStopEvent);
        shut(0);
    }
}

bool get_service_mode()
{
    return service_mode;
}

void WINAPI ServiceMain(DWORD argc, LPTSTR *argv)
{
    service_mode = true;
    (void)argc;
    (void)argv;

    g_StatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME, ServiceCtrlHandler);
    if (g_StatusHandle == NULL) return;

    g_ServiceStatus.dwServiceType      = SERVICE_WIN32_OWN_PROCESS;
    g_ServiceStatus.dwCurrentState     = SERVICE_START_PENDING;
    g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

    g_ServiceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (g_ServiceStopEvent == NULL)
    {
        g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
        return;
    }

    g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

    if (setjmp(g_exit_jmp) == 0)
    {
        work();
    }

    if (g_need_restart)
    {
        // 不报告 SERVICE_STOPPED，直接以非 0 码退出
        ExitProcess(1);
    }
    // 正常停止：报告 SERVICE_STOPPED
    CloseHandle(g_ServiceStopEvent);
    g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
}

int service_install()
{
    SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (!hSCManager)
    {
        fprintf(stderr, "[ERROR] 安装服务需要使用管理员权限\n");
        return -1;
    }

    char szPath[MAX_PATH];
    if (GetModuleFileName(NULL, szPath, MAX_PATH) == 0)
    {
        fprintf(stderr, "[ERROR] 无法获取程序路径\n");
        CloseServiceHandle(hSCManager);
        return -1;
    }

    SC_HANDLE hService = CreateService(
        hSCManager,
        SERVICE_NAME,
        DISPLAY_NAME,
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_AUTO_START,
        SERVICE_ERROR_NORMAL,
        szPath,
        NULL, NULL, NULL, NULL, NULL
    );

    if (!hService)
    {
        DWORD err = GetLastError();
        if (err == ERROR_SERVICE_EXISTS)
        {
            printf("[WARN] 服务已存在\n");
        }
        else
        {
            fprintf(stderr, "[ERROR] 服务注册失败，错误码: %lu\n", err);
            CloseServiceHandle(hSCManager);
            return -1;
        }
    }
    else
    {
        printf("[SUCCESS] 服务 '%s' 已安装并设置为开机自启\n", DISPLAY_NAME);
    }

    if (hService)
    {
        SERVICE_FAILURE_ACTIONS sfa;
        SC_ACTION actions[3];

        actions[0].Type  = SC_ACTION_RESTART;
        actions[0].Delay = 60000;

        actions[1].Type  = SC_ACTION_RESTART;
        actions[1].Delay = 60000;

        actions[2].Type  = SC_ACTION_NONE;
        actions[2].Delay = 0;

        sfa.dwResetPeriod = 86400;
        sfa.lpRebootMsg   = NULL;
        sfa.lpCommand     = NULL;
        sfa.cActions      = 3;
        sfa.lpsaActions   = actions;

        ChangeServiceConfig2(hService, SERVICE_CONFIG_FAILURE_ACTIONS, &sfa);
        printf("[INFO] 失败恢复操作已配置: 前两次失败后自动重启\n");
        CloseServiceHandle(hService);
    }

    hService = OpenService(hSCManager, SERVICE_NAME, SERVICE_START);
    if (hService)
    {
        if (StartService(hService, 0, NULL))
        {
            printf("[INFO] 服务已启动\n");
        }
        else
        {
            DWORD err = GetLastError();
            if (err == ERROR_SERVICE_ALREADY_RUNNING)
            {
                printf("[INFO] 服务已在运行中\n");
            }
            else
            {
                printf("[ERROR] 服务启动失败，错误码: %lu\n", err);
            }
        }
        CloseServiceHandle(hService);
    }
    else
    {
        printf("[ERROR] 无法打开服务以启动，错误码: %lu\n", GetLastError());
    }

    CloseServiceHandle(hSCManager);
    return 0;
}

int service_uninstall()
{
    SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (!hSCManager)
    {
        fprintf(stderr, "[ERROR] 无法打开服务控制管理器 (需要管理员权限)\n");
        return -1;
    }

    SC_HANDLE hService = OpenService(hSCManager, SERVICE_NAME, DELETE | SERVICE_STOP);
    if (!hService)
    {
        fprintf(stderr, "[ERROR] 服务不存在\n");
        CloseServiceHandle(hSCManager);
        return -1;
    }

    SERVICE_STATUS status;
    ControlService(hService, SERVICE_CONTROL_STOP, &status);

    if (DeleteService(hService))
    {
        printf("[SUCCESS] 服务 '%s' 已卸载\n", DISPLAY_NAME);
    }
    else
    {
        fprintf(stderr, "[ERROR] 服务卸载失败，错误码: %lu\n", GetLastError());
        CloseServiceHandle(hService);
        CloseServiceHandle(hSCManager);
        return -1;
    }

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);
    return 0;
}

#elif __linux__

#include <string.h>

#include <unistd.h>
#include <errno.h>

#define SERVICE_NAME "esurfingclient"

static int GetExecutablePath(char *buf, size_t size)
{
    ssize_t len = readlink("/proc/self/exe", buf, size - 1);
    if (len != -1)
    {
        buf[len] = '\0';
        return 0;
    }
    return -1;
}

int service_install()
{
    if (geteuid() != 0)
    {
        fprintf(stderr, "[ERROR] 安装服务需要 root 权限\n");
        return -1;
    }

    char execPath[512];
    if (GetExecutablePath(execPath, sizeof(execPath)) != 0)
    {
        fprintf(stderr, "[ERROR] 无法获取可执行文件路径\n");
        return -1;
    }

    char serviceContent[1024];
    int len = snprintf(serviceContent, sizeof(serviceContent),
        "[Unit]\n"
        "Description=ESurfingClient Auth Service\n"
        "After=network.target\n\n"
        "[Service]\n"
        "Type=simple\n"
        "ExecStart=%s\n"
        "Restart=on-failure\n"
        "RestartSec=5\n"
        "StartLimitBurst=3\n"
        "StartLimitIntervalSec=86400\n"
        "StandardOutput=journal\n"
        "StandardError=journal\n\n"
        "[Install]\n"
        "WantedBy=multi-user.target\n", execPath);

    if (len < 0 || (size_t)len >= sizeof(serviceContent))
    {
        fprintf(stderr, "[ERROR] 服务配置文件内容过长\n");
        return -1;
    }

    char serviceFilePath[256];
    snprintf(serviceFilePath, sizeof(serviceFilePath),
             "/etc/systemd/system/%s.service", SERVICE_NAME);

    FILE *fp = fopen(serviceFilePath, "w");
    if (fp == NULL)
    {
        perror("[ERROR] 无法写入 service 文件");
        return -1;
    }
    fputs(serviceContent, fp);
    fclose(fp);
    printf("[INFO] Systemd 服务文件已创建: %s\n", serviceFilePath);

    if (system("systemctl daemon-reload") != 0)
    {
        fprintf(stderr, "[ERROR] systemctl daemon-reload 失败\n");
        return -1;
    }

    char cmd[300];
    snprintf(cmd, sizeof(cmd), "systemctl enable %s.service", SERVICE_NAME);
    if (system(cmd) != 0)
    {
        fprintf(stderr, "[ERROR] 启用服务失败\n");
        return -1;
    }

    printf("[SUCCESS] 服务 '%s' 已安装并设置为开机自启\n", SERVICE_NAME);

    char startCmd[300];
    snprintf(startCmd, sizeof(startCmd), "systemctl start %s.service", SERVICE_NAME);
    if (system(startCmd) == 0)
    {
        printf("[INFO] 服务已启动\n");
    }
    else
    {
        printf("[ERROR] 服务启动失败\n");
    }
    return 0;
}

int service_uninstall()
{
    if (geteuid() != 0)
    {
        fprintf(stderr, "[ERROR] 卸载服务需要 root 权限\n");
        return -1;
    }

    char cmd[300];
    snprintf(cmd, sizeof(cmd), "systemctl stop %s.service 2>/dev/null", SERVICE_NAME);
    system(cmd);

    snprintf(cmd, sizeof(cmd), "systemctl disable %s.service 2>/dev/null", SERVICE_NAME);
    system(cmd);

    char serviceFilePath[256];
    snprintf(serviceFilePath, sizeof(serviceFilePath),
             "/etc/systemd/system/%s.service", SERVICE_NAME);

    if (remove(serviceFilePath) == 0)
    {
        printf("[INFO] Systemd 服务文件已删除\n");
    }
    else if (errno == ENOENT)
    {
        printf("[WARN] 服务文件不存在\n");
    }
    else
    {
        perror("[ERROR] 删除 service 文件失败");
        return -1;
    }

    system("systemctl daemon-reload");
    printf("[SUCCESS] 服务 '%s' 已卸载\n", SERVICE_NAME);
    return 0;
}

#elif __APPLE__

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <mach-o/dyld.h>

#define SERVICE_NAME "com.esurfingclient.auth"

static int GetExecutablePath(char *buf, size_t size)
{
    uint32_t bufsize = (uint32_t)size;
    if (_NSGetExecutablePath(buf, &bufsize) == 0)
    {
        return 0;
    }
    return -1;
}

int service_install()
{
    if (geteuid() != 0)
    {
        fprintf(stderr, "[ERROR] 安装服务需要 root 权限，请使用 sudo 运行\n");
        return -1;
    }

    char execPath[512];
    if (GetExecutablePath(execPath, sizeof(execPath)) != 0)
    {
        fprintf(stderr, "[ERROR] 无法获取可执行文件路径\n");
        return -1;
    }

    char plistPath[512];
    snprintf(plistPath, sizeof(plistPath),
             "/Library/LaunchDaemons/%s.plist", SERVICE_NAME);

    FILE *fp = fopen(plistPath, "w");
    if (fp == NULL)
    {
        perror("[ERROR] 无法写入 plist 文件");
        return -1;
    }

    fprintf(fp,
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" "
        "\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
        "<plist version=\"1.0\">\n"
        "<dict>\n"
        "    <key>Label</key>\n"
        "    <string>%s</string>\n"
        "    <key>ProgramArguments</key>\n"
        "    <array>\n"
        "        <string>%s</string>\n"
        "    </array>\n"
        "    <key>RunAtLoad</key>\n"
        "    <true/>\n"
        "    <key>KeepAlive</key>\n"
        "    <dict>\n"
        "        <key>Crashed</key>\n"
        "        <true/>\n"
        "        <key>SuccessfulExit</key>\n"
        "        <false/>\n"
        "    </dict>\n"
        "    <key>StandardOutPath</key>\n"
        "    <string>/var/log/%s.log</string>\n"
        "    <key>StandardErrorPath</key>\n"
        "    <string>/var/log/%s_error.log</string>\n"
        "</dict>\n"
        "</plist>\n",
        SERVICE_NAME, execPath, SERVICE_NAME, SERVICE_NAME);

    fclose(fp);
    printf("[INFO] LaunchDaemon plist 文件已创建: %s\n", plistPath);

    char cmd[512];
    snprintf(cmd, sizeof(cmd), "launchctl load %s", plistPath);
    if (system(cmd) == 0)
    {
        printf("[SUCCESS] 服务 '%s' 已安装并设置为开机自启\n", SERVICE_NAME);
        printf("[INFO] 服务已启动\n");
    }
    else
    {
        fprintf(stderr, "[ERROR] 服务加载失败\n");
        return -1;
    }

    return 0;
}

int service_uninstall()
{
    if (geteuid() != 0)
    {
        fprintf(stderr, "[ERROR] 卸载服务需要 root 权限，请使用 sudo 运行\n");
        return -1;
    }

    char plistPath[512];
    snprintf(plistPath, sizeof(plistPath),
             "/Library/LaunchDaemons/%s.plist", SERVICE_NAME);

    char cmd[512];
    snprintf(cmd, sizeof(cmd), "launchctl unload %s 2>/dev/null", plistPath);
    system(cmd);

    if (remove(plistPath) == 0)
    {
        printf("[SUCCESS] 服务 '%s' 已卸载\n", SERVICE_NAME);
    }
    else if (errno == ENOENT)
    {
        printf("[NOTE] 服务文件不存在，可能已被删除\n");
    }
    else
    {
        perror("[ERROR] 删除 plist 文件失败");
        return -1;
    }

    return 0;
}

#endif
