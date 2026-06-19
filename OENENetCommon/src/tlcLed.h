#pragma once
#include <Arduino.h>
#include <OSCMessage.h>

// -----------------------------------------------------------------------------
// TLC59711 LED ENGINE — PUBLIC API
// -----------------------------------------------------------------------------

// Configure logical LED layout
void tlcLedConfigure(int numRgb, int numLed);

// Build channel mapping (RGB first, then single LEDs)
void tlcLedInitMapping();

// Initialize TLC59711 hardware (auto driver count)
void tlcLedInitHardware(int clkPin, int dataPin);

// High‑level RGB fade API (base)
void tlcRgbLedFade(int rgbIndex,
                   uint16_t r, uint16_t g, uint16_t b,
                   uint32_t timeMs);

// High‑level internal API (same semantics as OSC, 0-based indices)
void tlcLed(int ledIndex, uint16_t value, uint32_t timeMs);
void tlcLedAll(uint16_t value, uint32_t timeMs);

void tlcRgb(int rgbIndex, uint16_t r, uint16_t g, uint16_t b, uint32_t timeMs);
void tlcRgbAll(uint16_t r, uint16_t g, uint16_t b, uint32_t timeMs);

// OSC router for TLC commands
void tlcOscRouter(OSCMessage &msg);

// Fade engine update (call in loop)
void tlcLedUpdate();

// -----------------------------------------------------------------------------
// ERROR CALLBACK
// -----------------------------------------------------------------------------

typedef void (*TlcErrorCallback)(const char* code);
void tlcLedSetErrorCallback(TlcErrorCallback cb);
