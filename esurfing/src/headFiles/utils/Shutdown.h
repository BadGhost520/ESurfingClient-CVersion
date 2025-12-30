#ifndef ESURFINGCLIENT_SHUTDOWN_H
#define ESURFINGCLIENT_SHUTDOWN_H

/**
 * 检查适配器线程是否需要关闭
 */
void checkAdapterStop();

/**
 * 关闭函数
 * @param exitCode 退出码
 */
void shut(int exitCode);

/**
 * 初始化关闭函数
 */
void initShutdown();

#endif //ESURFINGCLIENT_SHUTDOWN_H