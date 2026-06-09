#pragma once
#include <Arduino.h>

// -----------------------------------------------------------------------------
// pwmLed library
// PWM LED engine: gamma correction, fade interpolation, LEDC output.


// Initialize the PWM LED engine
// pins[] : array of physical GPIO pins
// count  : number of PWM channels
void pwmLedInit(const int* pins, int count);

// Update fade engine (must be called inside loop())
void pwmLedUpdate();

// Start a fade on a logical LED index
// ledIndex   : 0..count-1
// targetDuty : 0..4095 (logical duty before gamma correction)
// durationMs : fade duration in milliseconds
void pwmLedFade(int ledIndex, uint16_t targetDuty, uint32_t durationMs);
