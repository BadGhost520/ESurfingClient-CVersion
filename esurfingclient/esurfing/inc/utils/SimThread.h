#ifndef SIMPLE_THREAD_H
#define SIMPLE_THREAD_H

#include <stdint.h>

// 线程句柄类型
typedef struct SimThread sim_thread_t;

// 线程函数类型：void* 参数，int 返回值
typedef int (*sim_thread_func)(void* arg);

// 创建线程
sim_thread_t* sim_thread_create(sim_thread_func func, void* arg);

// 等待线程结束，获取返回值
int sim_thread_join(sim_thread_t* thread, int* exit_code);

// 分离线程(让线程结束后自动清理)
int sim_thread_detach(sim_thread_t* thread);

// 获取当前线程ID(用于调试)
uint64_t sim_thread_cur_id(void);

// 销毁线程句柄(不等待线程结束)
void sim_thread_destroy(sim_thread_t* thread);

#endif // SIMPLE_THREAD_H