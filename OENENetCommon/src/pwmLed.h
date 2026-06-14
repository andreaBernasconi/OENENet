#pragma once
#include <Arduino.h>
#include <OSCMessage.h>

// -----------------------------------------------------------------------------
// PWM LED ENGINE — PUBLIC API
// 12‑bit logical values (0..4095) + gamma + fade 
// -----------------------------------------------------------------------------


void pwmLedInit(const int* pins, int count);



void pwmLedFade(int ledIndex, uint16_t targetDuty, uint32_t durationMs);


void pwmLedUpdate();


void pwmOscRouter(OSCMessage &msg);


typedef void (*PwmErrorCallback)(const char* code);
void pwmLedSetErrorCallback(PwmErrorCallback cb);
