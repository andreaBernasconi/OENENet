//RoutingTable.h

#pragma once
#include <Arduino.h>
#include <stdint.h>   // uint8_t
#include <string.h>   // strcmp, memcmp
#include <Preferences.h>
extern Preferences prefs;
#define MAX_ROUTES 10



struct RouteEntry {
    const char* prefix; // short board name used in OSC addressing
    uint8_t mac[6]; // MAC address of the board
    unsigned long lastAlive;  // timestamp of last received /alive
};

extern RouteEntry routingTable[];
extern int routingTableSize;

RouteEntry* findRoute(const char* prefix);
const char* findPrefixByMac(const uint8_t* mac);
RouteEntry* findRouteByMac(const uint8_t* mac); 
bool findRouteSafe(const char* prefix, RouteEntry*& route, int& index);
void clearRoutingTable();
void freeRoutingTable();
bool isValidPrefix(const char* p);
bool isValidMac(const uint8_t mac[6]);
void saveRoutingTableToNVS();
void loadRoutingTableFromNVS();

