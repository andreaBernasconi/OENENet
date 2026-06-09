#pragma once
#include <Arduino.h>

// -----------------------------------------------------------------------------
// tlcLed library
// TLC59711 LED engine: gamma correction, fade interpolation, 16‑bit output.
// -----------------------------------------------------------------------------

// Initialize the TLC LED engine
// clkPin     : TLC59711 CLK pin
// dataPin    : TLC59711 MOSI/DIN pin
// numDrivers : number of TLC59711 chips in daisy chain
void tlcLedInit(int clkPin, int dataPin, int numDrivers);

// Update the fade engine (must be called inside loop())
void tlcLedUpdate();

// Start a fade on a logical channel
// ch      : channel index 0..(numDrivers*12 - 1)
// value   : logical brightness 0..4095 (before gamma correction)
// timeMs  : fade duration in milliseconds
void tlcLedFade(int ch, uint16_t value, uint32_t timeMs);

// Start a fade on all TLC channels simultaneously
// value   : logical brightness 0..4095 (before gamma correction)
// timeMs  : fade duration in milliseconds
void tlcLedFadeAll(uint16_t value, uint32_t timeMs);
