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

// Initialize TLC59711 hardware
void tlcLedInitHardware(int clkPin, int dataPin, int numDrivers);

// High‑level RGB fade API
void tlcRgbLedFade(int rgbIndex,
                   uint16_t r, uint16_t g, uint16_t b,
                   uint32_t timeMs);

// OSC router for TLC commands
void tlcOscRouter(OSCMessage &msg);

// Fade engine update (call in loop)
void tlcLedUpdate();

// -----------------------------------------------------------------------------
// ERROR CALLBACK
// -----------------------------------------------------------------------------

typedef void (*TlcErrorCallback)(const char* code);
void tlcLedSetErrorCallback(TlcErrorCallback cb);
