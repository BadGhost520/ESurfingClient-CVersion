#include "supervisor/supervisor_internal.h"

child_t s_children[SUPERVISOR_MAX_CHILDREN];
int s_child_count = 0;
stop_flag_t s_stop_requested = 0;
