#pragma once
#include <OSCMessage.h>

bool oscCopyArgs(OSCMessage &src, OSCMessage &dst);


OSCMessage oscBuildMessage(const char* address, const OSCMessage& src);

int buildOscForEspNow(OSCMessage &msg, uint8_t *buffer, int maxLen);


