#ifndef ESURFINGCLIENT_DASHBOARDHELPER_H
#define ESURFINGCLIENT_DASHBOARDHELPER_H

#include <stdbool.h>

/**
 * 随包的 Dashboard 辅助脚本 (Windows 才有, 见 tools/windows-network-panel/)
 *
 * 它负责两件事: 采样各网卡的实时网速写进 portal/assets/traffic.json、
 * 可选地在回环地址上开一个热点控制桥。网页上的"实时网速"与"热点"两张卡片
 * 全靠它, 所以由主程序自己拉起, 用户不用再单独双击一个脚本。
 *
 * 生命周期跟着主程序走: 启动时拉起, 关闭时先停它再退 (脚本不会自己退,
 * 留着就成了孤儿进程)。脚本的 stdout/stderr 逐行转发进本程序的日志文件,
 * 与主程序的日志共用同一份 run.log。
 */

/**
 * @brief 拉起 Dashboard 辅助脚本 (已经在跑时直接返回)
 *
 * 找不到脚本 (不是发布包/构建目录布局)、没有 portal/assets、不是 Windows 时
 * 返回 false 并只记一条日志 —— 这只是可选功能, 不该影响认证主流程。
 * @return 是否拉起了脚本
 */
bool dashboard_helper_start(void);

/**
 * @brief 停掉 Dashboard 辅助脚本并等它退出
 *
 * 先写停止信号文件让脚本自己收尾 (写完最后一帧 / 停桥接 / 删临时文件),
 * 超时没退才强杀。没启动过时是空操作, 可以放心在关闭流程里无条件调用。
 */
void dashboard_helper_stop(void);

#endif
