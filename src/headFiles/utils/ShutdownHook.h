//
// Created by bad_g on 2025/9/27.
//

#ifndef ESURFINGCLIENT_SHUTDOWNHOOK_H
#define ESURFINGCLIENT_SHUTDOWNHOOK_H

#ifdef __cplusplus
extern "C" {
#endif

    // 初始化shutdown hook
    void addShutdownHook(void);

    // 用户可以在这里添加自定义清理逻辑的函数指针
    extern void (*user_cleanup_function)(void);

#ifdef __cplusplus
}
#endif

#endif //ESURFINGCLIENT_SHUTDOWNHOOK_H