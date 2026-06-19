/*
Title: ESP32 TLC59711 Node
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

#include <tlcLed.h>

// -----------------------------------------------------------------------------
// TLC USER CONFIGURATION
// -----------------------------------------------------------------------------

#define TLC_NUM_RGB 4
#define TLC_NUM_LED 0

#define TLC_CLK 12
#define TLC_MOSI 11

// -----------------------------------------------------------------------------
// SETUP
// -----------------------------------------------------------------------------

void setup() {
    initCommon();

    // Configure TLC logical layout
    tlcLedConfigure(TLC_NUM_RGB, TLC_NUM_LED);

    // Build channel mapping
    tlcLedInitMapping();

    // Initialize TLC hardware (driver count auto-calculated)
    tlcLedInitHardware(TLC_CLK, TLC_MOSI);

    // Error callback using sendOscError()
    tlcLedSetErrorCallback([](const char* code){
        sendOscError("/error/tlc", code);
    });

    // OSC router
    setOscCallback(tlcOscRouter);
}

// -----------------------------------------------------------------------------
// LOOP
// -----------------------------------------------------------------------------

void loop() {
    tlcLedUpdate();
}
