#pragma once
#include <Preferences.h>
#include <esp_wifi.h>

struct RadioConfig {
    int channel;
    int txPower;
};

// DICHIARAZIONE DELLA VARIABILE GLOBALE
extern RadioConfig radioConfig;

void loadRadioConfig(RadioConfig &cfg);
void saveRadioConfig(const RadioConfig &cfg);
void applyRadioConfig(const RadioConfig &cfg);
