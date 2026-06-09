#pragma once
#include <Preferences.h>
#include <esp_wifi.h>

struct RadioConfig {
    int channel;
    int txPower;
};

void loadRadioConfig(RadioConfig &cfg);
void saveRadioConfig(const RadioConfig &cfg);
void applyRadioConfig(const RadioConfig &cfg);
