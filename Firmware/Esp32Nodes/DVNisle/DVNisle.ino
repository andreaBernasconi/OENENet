/*
Title: ESP32 CORE 
Board: OLIMEX ESP32‑S3‑DevKit‑LiPo 
USB CDC On Boot → Enabled
USB Mode → Hardware CDC and JTAG
Flash Size → 8MB
Flash Mode → QIO
Flash Frequency → 80 MHz
Partition Scheme → Default 8MB with spiffs
PSRAM → OPI PSRAM
CPU Frequency → 240 MHz
Upload Speed: 921600
*/

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <OSCMessage.h>
#include <Logging.h>
#include <OscUtils.h>
#include <Utils.h>
#include <RadioConfig.h>
#include <esp32core.h>

#include "pwmLed.h"
#include "tlcLed.h"

// --- PWM config ---
int pwmPins[] = {4, 7, 15, 16, 17, 18};
const int pwmCount = 6;

// --- TLC config ---
#define TLC_CLK      12
#define TLC_MOSI     11
#define TLC_NUM      1
#define TLC_CHANNELS (TLC_NUM * 12)

void handlePwmLed(OSCMessage &msg) {
    if (msg.size() < 3) return;

    int ledIndex = msg.getInt(0) - 1;
    uint16_t target = msg.getInt(1);
    uint32_t timeMs = msg.getInt(2);

    pwmLedFade(ledIndex, target, timeMs);
}

void handlePwmLedAll(OSCMessage &msg) {
    if (msg.size() < 2) return;

    uint16_t target = msg.getInt(0);
    uint32_t timeMs = msg.getInt(1);

    for (int i = 0; i < pwmCount; i++)
        pwmLedFade(i, target, timeMs);
}

void handleTlcLed(OSCMessage &msg) {
    if (msg.size() < 3) return;

    int ledIndex = msg.getInt(0) - 1;
    uint16_t target = msg.getInt(1);
    uint32_t timeMs = msg.getInt(2);

    if (ledIndex < 0 || ledIndex >= TLC_CHANNELS) return;
    if (target > 4095) target = 4095;

    tlcLedFade(ledIndex, target, timeMs);
}

void handleTlcLedAll(OSCMessage &msg) {
    if (msg.size() < 2) return;

    uint16_t target = msg.getInt(0);
    uint32_t timeMs = msg.getInt(1);

    if (target > 4095) target = 4095;

    tlcLedFadeAll(target, timeMs);
}



// --- OSC router ---
void oscRouter(OSCMessage &msg) {
  msg.dispatch("/pwmLed",    handlePwmLed);
  msg.dispatch("/pwmLedAll", handlePwmLedAll);
  msg.dispatch("/tlcLed",    handleTlcLed);
  msg.dispatch("/tlcLedAll", handleTlcLedAll);
}

void setup() {
  initCommon();

  setOscCallback(oscRouter);

  pwmLedInit(pwmPins, pwmCount);
  tlcLedInit(TLC_CLK, TLC_MOSI, TLC_NUM);
}


void loop() {
  pwmLedUpdate();
  tlcLedUpdate();
}