#pragma once

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <RadioConfig.h>
#include <OSCMessage.h>

// Maximum allowed ESP-NOW payload size
#define ESPNOW_MAX_PAYLOAD 250

// Default onboard status LED pin (can be overridden by the board)
#ifndef STATUS_LED_PIN
#define STATUS_LED_PIN 38
#endif



// Global radio configuration used by ESP32 core boards
extern RadioConfig coreRadioConfig;

// Last sender MAC address seen on ESP-NOW
extern uint8_t coreLastSenderMac[6];

// Emergency window state flag
extern bool coreEmergencyActive;

// Initialize ESP-NOW and register the generic receive callback
bool coreInitEspNow();

// Generic ESP-NOW receive callback
void coreOnDataRecv(const esp_now_recv_info_t *info,
                    const uint8_t *data,
                    int len);
// Common OSC handlers
void coreHandleAlive(OSCMessage &msg);
void coreHandleChannel(OSCMessage &msg);
void coreHandlePower(OSCMessage &msg);
void coreHandleRadioStatus(OSCMessage &msg);

// Run the emergency window (LED blink + optional board-specific visuals)
void coreEmergencyWindow(uint8_t ledPin, unsigned long durationMs);

// Optional board-specific emergency visuals (weak symbol)
extern void coreEmergencyVisual();

// User-defined OSC callback
typedef void (*CoreOscCallback)(OSCMessage &msg);

// Register callback from the .ino
void coreSetOscCallback(CoreOscCallback cb);

