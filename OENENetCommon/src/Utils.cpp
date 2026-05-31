#include "Utils.h"
#include <string.h>


bool parseOscAddress(const char *address,
                     char *prefix, int prefixSize,
                     char *command, int commandSize)
{
    if (!address || address[0] != '/') return false;

    const char *addr = address + 1;
    const char *slash = strchr(addr, '/');
    if (!slash) return false;

    int prefixLen = slash - addr;
    if (prefixLen <= 0 || prefixLen >= prefixSize) return false;

    strncpy(prefix, addr, prefixLen);
    prefix[prefixLen] = '\0';

    strncpy(command, slash, commandSize);
    command[commandSize - 1] = '\0';

    return true;
}






