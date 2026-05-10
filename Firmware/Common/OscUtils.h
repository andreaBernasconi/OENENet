#pragma once
#include <OSCMessage.h>

bool oscCopyArgs(OSCMessage &src, OSCMessage &dst);
int oscSerialize(OSCMessage &msg, uint8_t *buffer, int maxLen);

OSCMessage oscBuildMessage(const char* address, const OSCMessage& src);

