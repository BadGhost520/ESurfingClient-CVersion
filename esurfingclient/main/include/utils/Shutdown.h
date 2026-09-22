#ifndef ESURFINGCLIENT_SHUTDOWN_H
#define ESURFINGCLIENT_SHUTDOWN_H

#include <stdint.h>

/**
 * @brief 关闭函数
 * @param exit_code 退出码
 */
void shut(int8_t exit_code);

/**
 * @brief 初始化关闭函数
 */
void init_shutdown_hook();

#endif //ESURFINGCLIENT_SHUTDOWN_H
