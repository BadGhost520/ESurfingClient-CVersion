#ifndef ESURFINGCLIENT_DIALERCLIENT_H
#define ESURFINGCLIENT_DIALERCLIENT_H

#include <stdbool.h>

/**
 * @brief 工作函数
 */
void work(void);

/**
 * @brief 各角色入口的公共起步动作
 *
 * 取全局状态、装关闭钩子、起日志、打横幅 —— 单进程 / 认证进程 / Web 进程 /
 * 监管进程四处原本各抄了一份, 而且已经有细节漂移 (监管进程自己手写了横幅)。
 * @return 日志系统是否初始化成功 (失败时调用方按自己的方式退出)
 */
bool app_bootstrap(void);

#endif
