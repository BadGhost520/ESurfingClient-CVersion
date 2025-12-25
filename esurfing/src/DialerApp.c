#include "headFiles/cipher/CipherInterface.h"
#include "headFiles/webserver/WebServer.h"
#include "headFiles/utils/PlatformUtils.h"
#include "headFiles/utils/Logger.h"
#include "headFiles/Constants.h"
#include "headFiles/Session.h"
#include "headFiles/Client.h"
#include "headFiles/States.h"

void restart()
{
    checkLogLevel();
    if (dialer_adapter.runtime_status.is_initialized)
    {
        if (dialer_adapter.runtime_status.is_logged) term();
        cipherFactoryDestroy();
        sessionFree();
    }
    dialer_adapter.timestamp.auth_time = 0;
    sleepMilliseconds(5000);
    initConstants();
    refreshStates();
}

void dialerApp()
{
    dialer_adapter.runtime_status.is_running = 1;
    initConstants();
    refreshStates();
    LOG_INFO("程序启动中，序号: %d", dialer_adapter.auth_config.adapter);
    sleepMilliseconds(5000);
    while (dialer_adapter.runtime_status.is_running)
    {
        if (currentTimeMillis() - dialer_adapter.timestamp.auth_time >= 172200000 && dialer_adapter.timestamp.auth_time != 0)
        {
            if (is_settings_changed)
            {
                LOG_INFO("设置已更改，正在重启认证");
                is_settings_changed = 0;
            }
            LOG_DEBUG("当前时间戳(毫秒): %lld", currentTimeMillis());
            LOG_WARN("已登录 2870 分钟(1 天 23 小时 50 分钟)，为避免被远程服务器踢下线，正在重新进行认证");
            restart();
        }
        else if (is_settings_changed)
        {
            LOG_INFO("设置已更改，正在重启认证");
            restart();
            is_settings_changed = 0;
        }
        run();
    }
}