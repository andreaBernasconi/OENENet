#include "RadioConfig.h"

// DEFINIZIONE REALE DELLA VARIABILE GLOBALE
RadioConfig radioConfig;

// Local Preferences instance (not exposed outside this file)
static Preferences prefs;

// Load radio configuration from NVS
void loadRadioConfig(RadioConfig &cfg) {
    prefs.begin("radio", false);

    cfg.channel = prefs.getInt("channel", 6);   // default channel = 6
    cfg.txPower = prefs.getInt("txPower", 78);  // default txPower = 78 (19.5 dBm)

    prefs.end();
}

// Save radio configuration to NVS
void saveRadioConfig(const RadioConfig &cfg) {
    prefs.begin("radio", false);

    prefs.putInt("channel", cfg.channel);
    prefs.putInt("txPower", cfg.txPower);

    prefs.end();
}

// Apply radio configuration to ESP32 Wi‑Fi hardware
void applyRadioConfig(const RadioConfig &cfg) {
    // Set Wi‑Fi channel
    esp_wifi_set_channel(cfg.channel, WIFI_SECOND_CHAN_NONE);

    // Set TX power (0–78)
    esp_wifi_set_max_tx_power(cfg.txPower);
}
