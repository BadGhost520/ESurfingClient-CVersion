#ifndef ESURFINGCLIENT_CONTROLINTERNAL_H
#define ESURFINGCLIENT_CONTROLINTERNAL_H

#include "states/States.h"

#ifdef _WIN32
#include <winsock2.h>
typedef SOCKET ctl_sock_t;
#define CTL_INVALID_SOCK INVALID_SOCKET
#define ctl_close closesocket
#else
#include <string.h>
#include <stdlib.h>
typedef int ctl_sock_t;
#define CTL_INVALID_SOCK (-1)
#define ctl_close close
#endif

extern uint16_t s_client_port;

bool ctl_net_init();

size_t ctl_read_line(const ctl_sock_t sock, char* buf, const size_t buf_len);

#endif
