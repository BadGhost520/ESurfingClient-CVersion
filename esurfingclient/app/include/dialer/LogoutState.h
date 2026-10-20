#ifndef ESURFINGCLIENT_LOGOUTSTATE_H
#define ESURFINGCLIENT_LOGOUTSTATE_H

#include "core/States.h"

/**
 * 会话存档: 给"非优雅退出"之后补登出用
 *
 * 进程被强杀 (断电 / 看门狗 _exit / kill -9) 时跑不到 clean(), 会话会留在服务端
 * 在线状态, 下次启动只能干等服务器踢下线。这里在登录成功后把 term 报文需要的
 * 现场 (URL / algo_id / ticket / 客户端标识) 存到配置文件旁边, 登出成功后删掉;
 * 下次启动发现存档就补做一次登出。存档不含账号密码。
 */

/**
 * @brief 把当前会话的现场存下来 (登录成功后调用)
 * @param status 该账号的状态
 * @return 是否写成功
 */
bool logout_state_save(const prog_status_t* status);

/**
 * @brief 读回存档并填进 status
 * @param status 输出 (按存档里的账号序号匹配)
 * @return 是否存在可用的存档
 */
bool logout_state_load(prog_status_t* status);

/**
 * @brief 删掉存档 (登出成功后, 或补登出尝试过之后调用)
 * @param idx 账号序号
 */
void logout_state_clear(uint8_t idx);

#endif
