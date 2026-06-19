#pragma once
#include <Arduino.h>
#include <OSCMessage.h>

// Initialize PWM engine with physical pins
void pwmLedInit(const int* pins, int count);

// Configure logical LED layout (RGB + single LEDs)
void pwmLedConfigure(int numRgb, int numSingle);

// Build mapping tables (logical → physical PWM channels)
void pwmLedInitMapping();

// Fade a logical RGB LED
void pwmRgbLedFade(int rgbIndex,
                   uint16_t r, uint16_t g, uint16_t b,
                   uint32_t durationMs);

// Fade a logical single LED
void pwmSingleLedFade(int ledIndex,
                      uint16_t targetDuty,
                      uint32_t durationMs);

// High-level logical LED control (same semantics as OSC, but 0-based indices)
void pwmLed(int ledIndex, uint16_t value, uint32_t timeMs);
void pwmLedAll(uint16_t value, uint32_t timeMs);

void pwmRgb(int rgbIndex, uint16_t r, uint16_t g, uint16_t b, uint32_t timeMs);
void pwmRgbAll(uint16_t r, uint16_t g, uint16_t b, uint32_t timeMs);

// Update fade engine (must be called in loop)
void pwmLedUpdate();

// OSC router for PWM commands
void pwmOscRouter(OSCMessage &msg);

// Error callback
typedef void (*PwmErrorCallback)(const char* code);
void pwmLedSetErrorCallback(PwmErrorCallback cb);
