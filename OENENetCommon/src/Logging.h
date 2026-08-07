#pragma once

// Unified logging for all OENENet firmware

#ifndef OENENET_DEBUG
#define OENENET_DEBUG 1
#endif

#if OENENET_DEBUG
  #define LOG(x)    do { Serial.print(x); } while (0)
  #define LOGLN(x)  do { Serial.println(x); } while (0)
#else
  #define LOG(x)
  #define LOGLN(x)
#endif
