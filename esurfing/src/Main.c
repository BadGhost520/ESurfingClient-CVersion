#include <process.h>

#include "headFiles/utils/Logger.h"

int main()
{
#ifdef _WIN32
    system("chcp 65001 > nul");
#endif
    loggerInit();
}
