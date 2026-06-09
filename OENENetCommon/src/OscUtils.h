#pragma once
#include <OSCMessage.h>

bool parseOscAddress(const char *address,
                     char *prefix, int prefixSize,
                     char *command, int commandSize);

bool oscCopyArgs(OSCMessage &src, OSCMessage &dst);

OSCMessage oscBuildMessage(const char* address, const OSCMessage& src);

int buildOscForEspNow(OSCMessage &msg, uint8_t *buffer, int maxLen);


