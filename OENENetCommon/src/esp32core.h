#pragma once

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <RadioConfig.h>
#include <OSCMessage.h>

#define ESPNOW_MAX_PAYLOAD 250

#ifndef STATUS_LED_PIN
#define STATUS_LED_PIN 38
#endif

extern RadioConfig radioConfig;
extern uint8_t lastSenderMac[6];
extern bool emergencyActive;

void initCommon();

bool initEspNow();

void onDataRecv(const esp_now_recv_info_t *info,
                const uint8_t *data,
                int len);

void handleAlive(OSCMessage &msg);
void handleChannel(OSCMessage &msg);
void handlePower(OSCMessage &msg);
void handleRadioStatus(OSCMessage &msg);

void emergencyWindow(uint8_t ledPin, unsigned long durationMs);
extern void emergencyVisual();

typedef void (*OscCallback)(OSCMessage &msg);
void setOscCallback(OscCallback cb);
