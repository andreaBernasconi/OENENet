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
int pwmPins[] = {4, 7, 15, 16, 17, 18};
const int pwmCount = 6;
// OSC handler for PWM LED control
void handlePwmLed(OSCMessage &msg) {
  if (msg.size() < 3)
    return;

  int ledIndex = msg.getInt(0) - 1;
  uint16_t target = msg.getInt(1);
  uint32_t timeMs = msg.getInt(2);
  pwmLedFade(ledIndex, target, timeMs);
}
void handlePwmLedAll(OSCMessage &msg) {
  if (msg.size() < 2)
    return;

  uint16_t target = msg.getInt(0);
  uint32_t timeMs = msg.getInt(1);
 
  for (int i = 0; i < pwmCount; i++) {
    pwmLedFade(i, target, timeMs);
  }
}


// OSC router from esp32core → sketch
void coreOscRouter(OSCMessage &msg) {
  msg.dispatch("/pwmLed", handlePwmLed);
  msg.dispatch("/pwmLedAll", handlePwmLedAll);
}
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
  pwmLedInit(pwmPins, pwmCount);
  

}

void loop() {
    // Update PWM fade engine
  pwmLedUpdate();
}