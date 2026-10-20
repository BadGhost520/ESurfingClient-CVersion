#include "SupervisorInternal.h"

child_t g_children[SUPERVISOR_MAX_CHILDREN];
int g_child_count = 0;
stop_flag_t g_supervisor_stop = 0;
