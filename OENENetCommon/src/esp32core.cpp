#include "esp32core.h"
#include <Logging.h>
#include <Utils.h>
#include <OscUtils.h>

static OscCallback userOscCallback = nullptr;

RadioConfig radioConfig;
uint8_t lastSenderMac[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
bool emergencyActive = false;


void initCommon() {
    Serial.begin(115200);
    delay(300);

    WiFi.mode(WIFI_STA);
    esp_wifi_set_channel(6, WIFI_SECOND_CHAN_NONE);

    if (!initEspNow()) return;

    emergencyWindow(STATUS_LED_PIN, 8000);
    applyRadioConfig(radioConfig);
}


bool initEspNow() {
    loadRadioConfig(radioConfig);

    if (esp_now_init() != ESP_OK) {
        LOGLN("ESP-NOW init failed");
        return false;
    }

    esp_now_register_recv_cb(onDataRecv);
    return true;
}

void onDataRecv(const esp_now_recv_info_t *info,
                const uint8_t *data,
                int len)
{
    memcpy(lastSenderMac, info->src_addr, 6);
    ensurePeer(lastSenderMac, radioConfig.channel);

    if (len <= 0 || len > ESPNOW_MAX_PAYLOAD) return;

    static uint8_t temp[ESPNOW_MAX_PAYLOAD];
    memcpy(temp, data, len);

    OSCMessage msg;
    msg.fill(temp, len);

    if (!msg.hasError()) {
        msg.dispatch("/alive", handleAlive);
        msg.dispatch("/radio/channel", handleChannel);
        msg.dispatch("/radio/power", handlePower);
        msg.dispatch("/radio/status", handleRadioStatus);

        if (userOscCallback != nullptr)
            userOscCallback(msg);
    }
}

void handleAlive(OSCMessage &msg) {
    OSCMessage reply("/alive_ack");
    static uint8_t buffer[250];

    ensurePeer(lastSenderMac, radioConfig.channel);
    sendOscToEspNow(lastSenderMac, reply, buffer, ESPNOW_MAX_PAYLOAD);
}

void handleChannel(OSCMessage &msg) {
    int newChannel = msg.getInt(0);
    radioConfig.channel = newChannel;
    saveRadioConfig(radioConfig);

    if (!emergencyActive)
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

void handleRadioStatus(OSCMessage &msg) {
    static uint8_t buffer[250];

    OSCMessage ch("/radio/channel");
    ch.add(radioConfig.channel);
    sendOscToEspNow(lastSenderMac, ch, buffer, ESPNOW_MAX_PAYLOAD);

    OSCMessage pw("/radio/power");
    pw.add(radioConfig.txPower);
    sendOscToEspNow(lastSenderMac, pw, buffer, ESPNOW_MAX_PAYLOAD);
}

void __attribute__((weak)) emergencyVisual() {}

void emergencyWindow(uint8_t ledPin, unsigned long durationMs) {
    emergencyActive = true;

    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, HIGH);

    unsigned long start = millis();
    bool state = false;

    while (millis() - start < durationMs) {
        state = !state;
        digitalWrite(ledPin, state ? HIGH : LOW);
        emergencyVisual();
        delay(150);
    }

    digitalWrite(ledPin, HIGH);
    delay(5);
    emergencyActive = false;
}

void setOscCallback(OscCallback cb) {
    userOscCallback = cb;
}
