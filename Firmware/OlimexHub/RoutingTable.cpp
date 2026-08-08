//RoutingTable.cpp

#include "RoutingTable.h"

// Routing table containing all known boards.
// Each entry defines:
// - prefix: short name used in OSC addressing
// - mac: MAC address of the board
// - lastAlive: timestamp of the last received /alive message

RouteEntry routingTable[MAX_ROUTES];
int routingTableSize = 0;


RouteEntry *findRoute(const char *prefix)
{
    for (int i = 0; i < routingTableSize; i++)
    {
        if (strcmp(prefix, routingTable[i].prefix) == 0)
        {
            return &routingTable[i];
        }
    }
    return nullptr;
}

const char *findPrefixByMac(const uint8_t *mac)
{
    for (int i = 0; i < routingTableSize; i++)
    {
        if (memcmp(mac, routingTable[i].mac, 6) == 0)
        {
            return routingTable[i].prefix;
        }
    }
    return nullptr;
}

RouteEntry *findRouteByMac(const uint8_t *mac)
{
    for (int i = 0; i < routingTableSize; i++)
    {
        if (memcmp(mac, routingTable[i].mac, 6) == 0)
        {
            return &routingTable[i];
        }
    }
    return nullptr;
}

bool findRouteSafe(const char *prefix, RouteEntry *&route, int &index)
{
    if (!prefix || prefix[0] == '\0')
        return false;

    for (int i = 0; i < routingTableSize; i++)
    {
        if (strcmp(routingTable[i].prefix, prefix) == 0)
        {
            route = &routingTable[i];
            index = i;
            return true;
        }
    }

    return false;
}

void freeRoutingTable() {
    for (int i = 0; i < routingTableSize; i++) {
        if (routingTable[i].prefix != nullptr) {
            free((void*)routingTable[i].prefix);
            routingTable[i].prefix = nullptr;
        }
    }
}

void clearRoutingTable() {
    freeRoutingTable();
    routingTableSize = 0;
}

bool isValidPrefix(const char* p) {
    // Prefix must not be null or empty
    if (!p || p[0] == '\0') return false;

    // Prefix must be reasonably short
    if (strlen(p) > 31) return false;

    return true;
}

bool isValidMac(const uint8_t mac[6]) {
    bool allZero = true;
    bool allFF = true;

    for (int i = 0; i < 6; i++) {
        if (mac[i] != 0x00) allZero = false;
        if (mac[i] != 0xFF) allFF = false;
    }

    // Reject MACs that are all zeros or all FFs
    return !(allZero || allFF);
}

void saveRoutingTableToNVS() {
    prefs.begin("routing", false);  // open NVS namespace

    // Save table size
    prefs.putInt("size", routingTableSize);

    // Save each entry
    for (int i = 0; i < routingTableSize; i++) {

        // Save prefix
        String keyPrefix = "pfx" + String(i);
        prefs.putString(keyPrefix.c_str(), routingTable[i].prefix);

        // Save MAC
        String keyMac = "mac" + String(i);
        prefs.putBytes(keyMac.c_str(), routingTable[i].mac, 6);
    }

    prefs.end();
}

void loadRoutingTableFromNVS() {
    prefs.begin("routing", true);  // read-only

    int size = prefs.getInt("size", -1);

    if (size <= 0 || size > MAX_ROUTES) {
        prefs.end();
        return;  // no valid table stored
    }

    clearRoutingTable();  // free old prefixes

    routingTableSize = size;

    for (int i = 0; i < routingTableSize; i++) {

        // Load prefix
        String keyPrefix = "pfx" + String(i);
        String pfx = prefs.getString(keyPrefix.c_str(), "invalid");
        routingTable[i].prefix = strdup(pfx.c_str());

        // Load MAC
        String keyMac = "mac" + String(i);
        prefs.getBytes(keyMac.c_str(), routingTable[i].mac, 6);

        routingTable[i].lastAlive = 0;
    }

    prefs.end();
}
