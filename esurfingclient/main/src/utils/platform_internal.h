#ifndef ESURFINGCLIENT_PLATFORM_INTERNAL_H
#define ESURFINGCLIENT_PLATFORM_INTERNAL_H

#include "utils/PlatformUtils.h"

#ifdef _WIN32

#include <iphlpapi.h>

#else

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#ifndef ERANGE
#define ERANGE 34
#endif

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#endif

#endif
