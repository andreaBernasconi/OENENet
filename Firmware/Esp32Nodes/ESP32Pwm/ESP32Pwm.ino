/*
Title: ESP32 PWM
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

// -----------------------------------------------------------------------------
// USER CONFIGURATION
// -----------------------------------------------------------------------------

int pwmPins[] = {4, 7, 15, 16, 17, 18};
const int pwmCount = 6;

// -----------------------------------------------------------------------------
// ERROR CALLBACK
// -----------------------------------------------------------------------------

void sendPwmOscError(const char* code) {
    static uint8_t buffer[250];
    OSCMessage m("/error/pwmLed");
    m.add(code);
    sendOscToEspNow(lastSenderMac, m, buffer, ESPNOW_MAX_PAYLOAD);
}

// -----------------------------------------------------------------------------
// OSC ROUTER
// -----------------------------------------------------------------------------

void oscRouter(OSCMessage &msg) {
    // PWM commands
    pwmOscRouter(msg);
}



void setup() {
    initCommon();

    pwmLedInit(pwmPins, pwmCount);
    pwmLedSetErrorCallback(sendPwmOscError);

    setOscCallback(oscRouter);
}

void loop() {
    pwmLedUpdate();
}
