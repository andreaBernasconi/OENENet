/*
Title: DVNisle
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

// -----------------------------------------------------------------------------
// PWM CONFIGURATION
// -----------------------------------------------------------------------------

int pwmPins[] = {4, 7, 15, 16, 17, 18};
const int pwmCount = 6;

// -----------------------------------------------------------------------------
// TLC CONFIGURATION
// -----------------------------------------------------------------------------

#define NUM_RGB 8
#define NUM_LED 0

#define TLC_CLK 12
#define TLC_MOSI 11

#define CHANNELS_NEEDED (NUM_RGB * 3 + NUM_LED)
#define TLC_NUM ((CHANNELS_NEEDED + 11) / 12)

// -----------------------------------------------------------------------------
// ERROR CALLBACKS
// -----------------------------------------------------------------------------

void sendPwmOscError(const char* code) {
    static uint8_t buffer[250];
    OSCMessage m("/error/pwmLed");
    m.add(code);
    sendOscToEspNow(lastSenderMac, m, buffer, ESPNOW_MAX_PAYLOAD);
}

void sendTlcOscError(const char* code) {
    static uint8_t buffer[250];
    OSCMessage m("/error/tlc");
    m.add(code);
    sendOscToEspNow(lastSenderMac, m, buffer, ESPNOW_MAX_PAYLOAD);
}

// -----------------------------------------------------------------------------
// OSC ROUTER 
// -----------------------------------------------------------------------------

void oscRouter(OSCMessage &msg) {
    tlcOscRouter(msg);  
    pwmOscRouter(msg);  
}



void setup() {
    initCommon();

    // PWM engine
    pwmLedInit(pwmPins, pwmCount);
    pwmLedSetErrorCallback(sendPwmOscError);

    // TLC engine
    tlcLedConfigure(NUM_RGB, NUM_LED);
    tlcLedInitMapping();
    tlcLedInitHardware(TLC_CLK, TLC_MOSI, TLC_NUM);
    tlcLedSetErrorCallback(sendTlcOscError);

    // Unified OSC router
    setOscCallback(oscRouter);
}

void loop() {
    pwmLedUpdate();
    tlcLedUpdate();
}
