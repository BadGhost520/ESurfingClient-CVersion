#include "control/Control.h"

#include "states/States.h"

#ifdef _WIN32
jmp_buf g_exit_jmp;
#else
#include <stddef.h>
#endif

uint64_t g_start_run_tm = 0;

prog_role_t g_prog_role = ROLE_STANDALONE;

uint8_t g_prog_account = 0;

uint16_t g_control_port = CONTROL_DEFAULT_PORT;

uint16_t g_web_port = DEFAULT_WEB_PORT;

bool g_web_external_acc = DEFAULT_WEB_EXTERNAL_ACC;

char g_control_token[CONTROL_TOKEN_LEN] = {0};

int g_main_argc = 0;

char** g_main_argv = NULL;

volatile sig_atomic_t g_stop_requested = 0;

int8_t g_prog_cnt = 0;

_Thread_local int8_t tl_thread_idx = -1;

_Thread_local const char* tl_thread_name = NULL;

prog_status_t* g_prog_status;

char g_school_network_symbol[SCHOOL_NETWORK_SYMBOL] = {0};

bool g_thread_keep_alive = false;

bool g_is_webserver_running = false;

bool g_need_exit = false;

bool g_prog_enabled = false;

bool g_need_restart = false;

bool g_cfg_loaded = false;

long g_conn_timeout = DEFAULT_CONN_TIMEOUT;

long g_op_timeout = DEFAULT_OP_TIMEOUT;
