#include "utils/SimThread.h"
#include <stdlib.h>

#ifdef _WIN32
    #include <windows.h>
    
    struct SimThread {
        HANDLE handle;
        DWORD id;
    };
    
    static DWORD WINAPI win32_thread_func(LPVOID arg) {
        sim_thread_func* func_info = (sim_thread_func*)arg;
        sim_thread_func func = func_info[0];
        void* func_arg = func_info[1];
        free(func_info);
        return (DWORD)func(func_arg);
    }
    
    sim_thread_t* sim_thread_create(sim_thread_func func, void* arg) {
        sim_thread_t* thread = (sim_thread_t*)malloc(sizeof(sim_thread_t));
        if (!thread) return NULL;
        
        sim_thread_func* func_info = (sim_thread_func*)malloc(2 * sizeof(sim_thread_func));
        if (!func_info) {
            free(thread);
            return NULL;
        }
        func_info[0] = func;
        func_info[1] = arg;
        
        thread->handle = CreateThread(NULL, 0, win32_thread_func, func_info, 0, &thread->id);
        if (!thread->handle) {
            free(func_info);
            free(thread);
            return NULL;
        }
        return thread;
    }
    
    int sim_thread_join(sim_thread_t* thread, int* exit_code) {
        if (!thread) return -1;
        if (WaitForSingleObject(thread->handle, INFINITE) != WAIT_OBJECT_0) {
            return -1;
        }
        if (exit_code) {
            DWORD code;
            GetExitCodeThread(thread->handle, &code);
            *exit_code = (int)code;
        }
        CloseHandle(thread->handle);
        thread->handle = NULL;
        return 0;
    }
    
    int sim_thread_detach(sim_thread_t* thread) {
        if (!thread) return -1;
        CloseHandle(thread->handle);
        thread->handle = NULL;
        return 0;
    }
    
    uint64_t sim_thread_cur_id(void) {
        return (uint64_t)GetCurrentThreadId();
    }
    
    void sim_thread_destroy(sim_thread_t* thread) {
        if (thread) {
            if (thread->handle) CloseHandle(thread->handle);
            free(thread);
        }
    }
    
#else
    #include <pthread.h>
    #include <unistd.h>
    
    struct SimThread {
        pthread_t thread;
        int detached;
    };
    
    static void* pthread_thread_func(void* arg) {
        sim_thread_func* func_info = (sim_thread_func*)arg;
        sim_thread_func func = func_info[0];
        void* func_arg = func_info[1];
        free(func_info);
        return (void*)(intptr_t)func(func_arg);
    }
    
    sim_thread_t* sim_thread_create(sim_thread_func func, void* arg) {
        sim_thread_t* thread = (sim_thread_t*)malloc(sizeof(sim_thread_t));
        if (!thread) return NULL;
        
        sim_thread_func* func_info = (sim_thread_func*)malloc(2 * sizeof(sim_thread_func));
        if (!func_info) {
            free(thread);
            return NULL;
        }
        func_info[0] = func;
        func_info[1] = arg;
        
        thread->detached = 0;
        if (pthread_create(&thread->thread, NULL, pthread_thread_func, func_info) != 0) {
            free(func_info);
            free(thread);
            return NULL;
        }
        return thread;
    }
    
    int sim_thread_join(sim_thread_t* thread, int* exit_code) {
        if (!thread) return -1;
        if (thread->detached) return -1;
        
        void* ret;
        if (pthread_join(thread->thread, &ret) != 0) return -1;
        if (exit_code) *exit_code = (int)(intptr_t)ret;
        return 0;
    }
    
    int sim_thread_detach(sim_thread_t* thread) {
        if (!thread) return -1;
        if (thread->detached) return -1;
        
        if (pthread_detach(thread->thread) != 0) return -1;
        thread->detached = 1;
        return 0;
    }
    
    uint64_t sim_thread_cur_id(void) {
        return (uint64_t)pthread_self();
    }
    
    void sim_thread_destroy(sim_thread_t* thread) {
        if (thread) free(thread);
    }
#endif