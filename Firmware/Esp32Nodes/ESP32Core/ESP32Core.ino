/*
Title: ESP32 CORE 
Board: ESP32Dev Module 
*/

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <OSCMessage.h>
#include <Logging.h>
#include <OscUtils.h>
#include <RadioConfig.h>

RadioConfig radioConfig;

bool emergencyWindow = false;

#define ESPNOW_MAX_PAYLOAD 250

uint8_t lastSenderMac[6];

void sendOscThruEspNow(OSCMessage &msg) {
  uint8_t buffer[ESPNOW_MAX_PAYLOAD];

  int len = buildOscForEspNow(msg, buffer, ESPNOW_MAX_PAYLOAD);
  if (len <= 0) {
    LOGLN("buildOscForEspNow failed");
    return;
  }
  esp_now_send(lastSenderMac, buffer, len);
}

// -----------------------------------------------------------------------------
// Basic OSC handlers
// -----------------------------------------------------------------------------
void handleAlive(OSCMessage &msg) {
  OSCMessage reply("/alive_ack");
  sendOscThruEspNow(reply);
}

void handleChannel(OSCMessage &msg) {
  int newChannel = msg.getInt(0);
  radioConfig.channel = newChannel;
  saveRadioConfig(radioConfig);

  if (emergencyWindow) return;

  applyRadioConfig(radioConfig);
  LOG("Radio channel → ");
  LOGLN(newChannel);
}

void handlePower(OSCMessage &msg) {
  int newPower = msg.getInt(0);
  radioConfig.txPower = newPower;
  saveRadioConfig(radioConfig);
  applyRadioConfig(radioConfig);
  LOG("Radio txPower → ");
  LOGLN(newPower);
}

// -----------------------------------------------------------------------------
// ESP-NOW receive callback
// -----------------------------------------------------------------------------
void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  memcpy(lastSenderMac, info->src_addr, 6);

  // Auto-add peer if missing
  if (!esp_now_is_peer_exist(lastSenderMac)) {
    esp_now_peer_info_t peer{};
    memcpy(peer.peer_addr, lastSenderMac, 6);
    peer.channel = 0;
    peer.encrypt = false;
    esp_now_add_peer(&peer);
  }

  if (len <= 0 || len > ESPNOW_MAX_PAYLOAD) return;

  static uint8_t temp[ESPNOW_MAX_PAYLOAD];
  memcpy(temp, data, len);

  OSCMessage msg;
  msg.fill(temp, len);

  if (!msg.hasError()) {
    msg.dispatch("/alive", handleAlive);
    msg.dispatch("/channel", handleChannel);
    msg.dispatch("/power", handlePower);
  }
}
// -----------------------------------------------------------------------------
// Setup
// -----------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(300);

  loadRadioConfig(radioConfig);

  WiFi.mode(WIFI_STA);

  emergencyWindow = true;

  esp_wifi_set_channel(6, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    LOGLN("ESP-NOW init failed");
    return;
  }

  esp_now_register_recv_cb(onDataRecv);

  OSCMessage msg("/emergency");
  msg.add(1);  // 1 = emergency window active
  sendOscThruEspNow(msg);

  // 5‑second emergency window
  unsigned long start = millis();
  while (millis() - start < 5000) {
    delay(10);
  }

  emergencyWindow = false;

  OSCMessage msg2("/emergency");
  msg2.add(0);  // 0 = emergency window finished
  sendOscThruEspNow(msg2);

  applyRadioConfig(radioConfig);
}




void loop() {
}