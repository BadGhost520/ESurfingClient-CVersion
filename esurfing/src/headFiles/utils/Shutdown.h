#ifndef ESURFINGCLIENT_SHUTDOWN_H
#define ESURFINGCLIENT_SHUTDOWN_H

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