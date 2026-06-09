/*
Title: ESP32 Tlc Adafruit TLC59711
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
#include <math.h>

#include <tlcLed.h>

#define TLC_CLK      12
#define TLC_MOSI     11
#define TLC_NUM      1
#define TLC_CHANNELS (TLC_NUM * 12)

// ---------------- OSC HANDLERS ----------------

void handleTlcLed(OSCMessage &msg) {
  if (msg.size() < 3)
    return;

  int      ledIndex = msg.getInt(0) - 1;   // TLC LED numbering starts at 1
  uint16_t target   = msg.getInt(1);       // 0–4095 (LOGICO)
  uint32_t timeMs   = msg.getInt(2);       // fade time

  if (ledIndex < 0 || ledIndex >= TLC_CHANNELS)
    return;

  if (target > 4095)
    target = 4095;

  tlcLedFade(ledIndex, target, timeMs);
}

void handleTlcLedAll(OSCMessage &msg) {
  if (msg.size() < 2)
    return;

  uint16_t target = msg.getInt(0);   // 0–4095
  uint32_t timeMs = msg.getInt(1);   // fade duration

  if (target > 4095)
    target = 4095;

  tlcLedFadeAll(target, timeMs);
}

void coreOscRouter(OSCMessage &msg) {
  msg.dispatch("/tlcLed", handleTlcLed);
  msg.dispatch("/tlcLedAll", handleTlcLedAll);
}

// ---------------- SETUP / LOOP ----------------

void setup() {
  Serial.begin(115200);
  delay(300);

  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(6, WIFI_SECOND_CHAN_NONE);

  if (!coreInitEspNow()) {
    return;
  }

  coreEmergencyWindow(STATUS_LED_PIN, 8000);
  applyRadioConfig(coreRadioConfig);

  coreSetOscCallback(coreOscRouter);

  tlcLedInit(TLC_CLK, TLC_MOSI, TLC_NUM);

  LOGLN("ESP32Tlc ready");
}

void loop() {
  tlcLedUpdate();
}
