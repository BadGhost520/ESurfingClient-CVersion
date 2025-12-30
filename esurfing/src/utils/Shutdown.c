#include <signal.h>
#include <stdlib.h>

#include "../headFiles/webserver/WebServer.h"
#include "../headFiles/utils/Shutdown.h"

#include <pthread.h>

#include "../headFiles/utils/Logger.h"
#include "../headFiles/DialerClient.h"
#include "../headFiles/Session.h"
#include "../headFiles/States.h"

static void signalHandler(const int sig)
{
    switch(sig)
    {
    case SIGINT:
        LOG_DEBUG("接收到 SIGINT 信号 (Ctrl+C)");
        shut(0);
        break;
    case SIGTERM:
        LOG_DEBUG("接收到 SIGTERM 信号 (Terminate request)");
        shut(0);
        break;
    default:
        LOG_DEBUG("接收到未处理的信号: %d", sig);
        shut(0);
    }
}

static void mainStop()
{
    LOG_DEBUG("执行主线程关闭函数");
    if (is_webserver_running) stopWebServer();
    loggerCleanup();
}

static void freeThread(AuthConfig* config)
{
    free(config->mac_address);
    free(config->ticket_url);
    free(config->client_id);
    free(config->school_id);
    free(config->auth_url);
    free(config->user_ip);
    free(config->algo_id);
    free(config->domain);
    free(config->ticket);
    free(config->ac_ip);
    free(config->area);
    config->mac_address = NULL;
    config->ticket_url = NULL;
    config->client_id = NULL;
    config->school_id = NULL;
    config->auth_url = NULL;
    config->user_ip = NULL;
    config->algo_id = NULL;
    config->domain = NULL;
    config->ticket = NULL;
    config->ac_ip = NULL;
    config->area = NULL;
}

static void resetThread(RuntimeStatus* runtime_status)
{
    runtime_status->is_settings_changed = false;
    runtime_status->is_initialized = false;
    runtime_status->is_running = false;
    runtime_status->is_authed = false;
}

static void cleanThread()
{
    resetThread(&thread_status[thread_index].dialer_context.runtime_status);
    freeThread(&thread_status[thread_index].dialer_context.auth_config);
    thread_status[thread_index].dialer_context.auth_time = 0;
    thread_status[thread_index].thread_is_running = false;
    thread_status[thread_index].thread_status = 0;
    thread_status[thread_index].need_stop = false;
}

static void adapterStop()
{
    LOG_DEBUG("执行线程关闭函数");
    if (thread_status[thread_index].dialer_context.runtime_status.is_initialized)
    {
        if (thread_status[thread_index].dialer_context.runtime_status.is_authed) term();
        freeSession();
    }
    cleanThread();
}

void checkAdapterStop()
{
    if (thread_status[thread_index].need_stop)
    {
        thread_status[thread_index].need_stop = false;
        adapterStop();
    }
}

void shut(const int exitCode)
{
    if (thread_status[0].thread_is_running || thread_status[1].thread_is_running)
    {
        LOG_INFO("等待子线程关闭");
        for (int i = 0; i < MAX_DIALER_COUNT; i++) thread_status[i].need_stop = true;
        sleepMilliseconds(5000);
    }
    LOG_INFO("关闭主线程");
    mainStop();
    exit(exitCode);
}

void initShutdown()
{
    if (signal(SIGINT, signalHandler) == SIG_ERR)
    {
        LOG_ERROR("信号 SIGINT");
        exit(1);
    }

    if (signal(SIGTERM, signalHandler) == SIG_ERR)
    {
        LOG_ERROR("信号 SIGTERM");
        exit(1);
    }
}
