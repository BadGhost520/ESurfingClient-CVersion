#ifndef ESURFINGCLIENT_LOGOUTSTATE_H
#define ESURFINGCLIENT_LOGOUTSTATE_H

#include "states/States.h"

/**
 * 会话存档 - 给"非优雅退出"之后补登出用
 *
 * 为什么需要它:
 *   进程被强杀时跑不到 clean(), 会话会留在服务端在线状态。哪些情况会这样:
 *     - 断电 / 重启
 *     - 看门狗判定卡死 (- 它只能 _exit, 来不及登出)
 *     - 控制端口被别的程序占用, 监管者只能硬杀
 *     - 管道里直接 kill -9
 *   下次起来的现象: 网络是通的, 但账号还挂在服务端, 于是日志一直刷
 *   "已连接至互联网", 要等服务器把会话踢下线才能重新认证 ——
 *   这段时间用户上不了网, 而且很不优雅。
 *
 * 做法:
 *   登录成功后把"以后要登出所需要的现场"存到磁盘, 登出成功后删掉;
 *   下次启动时若发现存档还在, 先补做一次登出再走正常流程。
 *
 * 存的是 term 报文用得着的字段 (URL / algo_id / ticket / 客户端标识) 与
 * 加解密工厂需要的 algo_id。【不含账号密码】—— term 报文本来就不需要它们,
 * 加解密工厂也能由 algo_id 完全重建。
 *
 * 存档放在配置文件旁边 (OpenWrt 上是 /etc/config/, 桌面是程序目录),
 * 这样断电重启之后也还在。
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
