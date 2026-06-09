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
  
}

void loop() {
}