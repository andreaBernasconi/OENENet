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

#define NUM_RGB 4
#define NUM_LED 0

#define TLC_CLK 12
#define TLC_MOSI 11

#define CHANNELS_NEEDED (NUM_RGB * 3 + NUM_LED)
#define TLC_NUM ((CHANNELS_NEEDED + 11) / 12)

// -----------------------------------------------------------------------------
// ERROR CALLBACK
// -----------------------------------------------------------------------------

void sendTlcOscError(const char* code) {
    static uint8_t buffer[250];
    OSCMessage m("/error/tlc");
    m.add(code);
    sendOscToEspNow(lastSenderMac, m, buffer, 250);
}



void setup() {
    initCommon();

    tlcLedConfigure(NUM_RGB, NUM_LED);
    tlcLedInitMapping();
    tlcLedInitHardware(TLC_CLK, TLC_MOSI, TLC_NUM);

    tlcLedSetErrorCallback(sendTlcOscError);

    setOscCallback(tlcOscRouter);
}


void loop() {
    tlcLedUpdate();
}
