#ifndef ESURFINGCLIENT_SIMTHREAD_H
#define ESURFINGCLIENT_SIMTHREAD_H

#include <stdint.h>

typedef struct SimThread sim_thread_t;

// 线程函数类型：void* 参数，int 返回值
typedef int (*sim_thread_func)(void* arg);

// 创建线程
sim_thread_t* sim_thread_create(sim_thread_func func, void* arg);

/**
 * @brief 创建线程, 失败时重试若干次 (瞬时资源不足很常见)
 *
 * 每次失败都会打一行 LOG_ERROR; 全部失败返回 NULL, 具体怎么处置由调用方决定
 * @param func 线程入口
 * @param arg 参数
 * @param retries 额外重试次数 (总尝试 = retries + 1)
 */
sim_thread_t* sim_thread_create_retry(sim_thread_func func, void* arg, int retries);

// 等待线程结束, 获取返回值
int sim_thread_join(sim_thread_t* thread, int* exit_code);

// 分离线程 (让线程结束后自动清理)
int sim_thread_detach(sim_thread_t* thread);

// 获取当前线程ID (用于调试)
uint64_t sim_thread_cur_id(void);

// 销毁线程句柄 (不等待线程结束)
void sim_thread_destroy(sim_thread_t* thread);

#endif
