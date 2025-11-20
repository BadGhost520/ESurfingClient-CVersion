#include <signal.h>
//
// Created by bad_g on 2025/9/27.
//
char* usr;
char* pwd;
char* chn;
volatile sig_atomic_t isDebug = 0;
volatile sig_atomic_t smallDevice = 0;