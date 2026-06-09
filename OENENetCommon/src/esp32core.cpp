#include "esp32core.h"
#include <Logging.h>
#include <Utils.h>
#include <OscUtils.h>

static CoreOscCallback userOscCallback = nullptr;


// Global radio configuration used by ESP32 core boards
RadioConfig coreRadioConfig;

// Last sender MAC address seen on ESP-NOW
uint8_t coreLastSenderMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Emergency window state flag
bool coreEmergencyActive = false;

// -----------------------------------------------------------------------------
// Initialize ESP-NOW and register the generic receive callback
// -----------------------------------------------------------------------------
bool coreInitEspNow() {
    // Load radio configuration from NVS
    loadRadioConfig(coreRadioConfig);

    if (esp_now_init() != ESP_OK) {
        LOGLN("ESP-NOW init failed");
        return false;
    }

    esp_now_register_recv_cb(coreOnDataRecv);
    return true;
}


// -----------------------------------------------------------------------------
// Generic ESP-NOW receive callback
// -----------------------------------------------------------------------------
void coreOnDataRecv(const esp_now_recv_info_t *info,
                    const uint8_t *data,
                    int len)
{
    // Store sender MAC address
    memcpy(coreLastSenderMac, info->src_addr, 6);

    // Ensure peer exists
    ensurePeer(coreLastSenderMac, coreRadioConfig.channel);

    // Validate payload size
    if (len <= 0 || len > ESPNOW_MAX_PAYLOAD) return;

    // Copy payload into a temporary buffer
    static uint8_t temp[ESPNOW_MAX_PAYLOAD];
    memcpy(temp, data, len);

    // Parse OSC message
    OSCMessage msg;
    msg.fill(temp, len);

    if (!msg.hasError()) {
       // Internal handlers (always active)
msg.dispatch("/alive", coreHandleAlive);
msg.dispatch("radio/channel", coreHandleChannel);
msg.dispatch("/radio/power", coreHandlePower);
msg.dispatch("/radio/status", coreHandleRadioStatus);


// Forward OSC message to the .ino sketch
if (userOscCallback != nullptr) {
    userOscCallback(msg);
}



    }
}
// -----------------------------------------------------------------------------
// Common OSC handlers
// -----------------------------------------------------------------------------
void coreHandleAlive(OSCMessage &msg) {
    OSCMessage reply("/alive_ack");
    static uint8_t buffer[250];

    ensurePeer(coreLastSenderMac, coreRadioConfig.channel);
    sendOscToEspNow(coreLastSenderMac, reply, buffer, ESPNOW_MAX_PAYLOAD);
}

void coreHandleChannel(OSCMessage &msg) {
    int newChannel = msg.getInt(0);
    coreRadioConfig.channel = newChannel;
    saveRadioConfig(coreRadioConfig);

    if (coreEmergencyActive) return;

    applyRadioConfig(coreRadioConfig);
    LOG("Radio channel → ");
    LOGLN(newChannel);
}

void coreHandlePower(OSCMessage &msg) {
    int newPower = msg.getInt(0);
    coreRadioConfig.txPower = newPower;
    saveRadioConfig(coreRadioConfig);

    applyRadioConfig(coreRadioConfig);
    LOG("Radio txPower → ");
    LOGLN(newPower);
}

void coreHandleRadioStatus(OSCMessage &msg) {
    static uint8_t buffer[250];

    // Send current channel
    OSCMessage ch("/radio/channel");
    ch.add(coreRadioConfig.channel);
    sendOscToEspNow(coreLastSenderMac, ch, buffer, ESPNOW_MAX_PAYLOAD);

    // Send current power
    OSCMessage pw("/radio/power");
    pw.add(coreRadioConfig.txPower);
    sendOscToEspNow(coreLastSenderMac, pw, buffer, ESPNOW_MAX_PAYLOAD);
}

// -----------------------------------------------------------------------------
// Optional board-specific emergency visuals (weak symbol)
// -----------------------------------------------------------------------------
void __attribute__((weak)) coreEmergencyVisual() {
    // Default: no board-specific visuals
}

// -----------------------------------------------------------------------------
// Emergency window (LED blink + optional visuals)
// -----------------------------------------------------------------------------
void coreEmergencyWindow(uint8_t ledPin, unsigned long durationMs) {
    coreEmergencyActive = true;

    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, HIGH);

    unsigned long start = millis();
    bool state = false;

    while (millis() - start < durationMs) {
        // Toggle LED
        state = !state;
        digitalWrite(ledPin, state ? HIGH : LOW);

        // Call optional board-specific visuals
        coreEmergencyVisual();

        delay(150);
    }

    digitalWrite(ledPin, HIGH);
    delay(5);
    coreEmergencyActive = false;
}

void coreSetOscCallback(CoreOscCallback cb) {
    userOscCallback = cb;
}
