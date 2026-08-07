#pragma once
#include <Arduino.h>
#include <OSCMessage.h>



bool ensurePeer(const uint8_t mac[6]);
bool sendEspNow(const uint8_t mac[6], const uint8_t *data, size_t len);
bool sendOscToEspNow(const uint8_t mac[6], OSCMessage &msg,
                     uint8_t *buffer, size_t maxLen);
