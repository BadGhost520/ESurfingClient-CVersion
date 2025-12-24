#include "headFiles/cipher/CipherInterface.h"
#include "headFiles/webserver/WebServer.h"
#include "headFiles/utils/PlatformUtils.h"
#include "headFiles/utils/Shutdown.h"
#include "headFiles/utils/Logger.h"
#include "headFiles/Constants.h"
#include "headFiles/Session.h"
#include "headFiles/Client.h"
#include "headFiles/States.h"

void restart()
{
    checkLogLevel();
    if (isInitialized)
    {
        if (isLogged) term();
        cipherFactoryDestroy();
        sessionFree();
    }
    authTime = 0;
    sleepMilliseconds(5000);
    initConstants();
    refreshStates();
}

void dialerApp()
{
    isRunning = 1;
    initShutdown();
    initConstants();
    refreshStates();
    LOG_INFO("程序启动中");
    sleepMilliseconds(5000);
    startWebServer();
    while (isRunning)
    {
        if (currentTimeMillis() - authTime >= 172200000 && authTime != 0)
        {
            if (isSettingsChange)
            {
                LOG_INFO("设置已更改，正在重启认证");
                isSettingsChange = 0;
            }
            LOG_DEBUG("当前时间戳(毫秒): %lld", currentTimeMillis());
            LOG_WARN("已登录 2870 分钟(1 天 23 小时 50 分钟)，为避免被远程服务器踢下线，正在重新进行认证");
            restart();
        }
        else if (isSettingsChange)
        {
            LOG_INFO("设置已更改，正在重启认证");
            restart();
            isSettingsChange = 0;
        }
        run();
    }
}