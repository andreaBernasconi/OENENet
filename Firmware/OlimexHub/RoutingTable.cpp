#include "RoutingTable.h"

// Routing table containing all known boards.
// Each entry defines:
// - prefix: short name used in OSC addressing
// - mac: MAC address of the board
// - enabled: whether the board should be used by the router
// - description: human‑readable label

RouteEntry routingTable[] = {
    { "scheda01", {0xa0, 0xb7, 0x65, 0x24, 0xa8, 0x20}, true, 0 },
    { "scheda02", {0xa0, 0xb7, 0x65, 0x26, 0x8f, 0xf8}, true, 0 },
    { "scheda03", {0x24,0x6F,0x28,0xAA,0xBB,0x03}, false, 0 },
    { "scheda04", {0x24,0x6F,0x28,0xAA,0xBB,0x04}, false, 0 }
};


const int routingTableSize = sizeof(routingTable) / sizeof(RouteEntry);

RouteEntry* findRoute(const char* prefix) {
    for (int i = 0; i < routingTableSize; i++) {
        if (routingTable[i].enabled && strcmp(prefix, routingTable[i].prefix) == 0) {
            return &routingTable[i];
        }
    }
    return nullptr;
}

const char* findPrefixByMac(const uint8_t* mac) {
    for (int i = 0; i < routingTableSize; i++) {
        if (memcmp(mac, routingTable[i].mac, 6) == 0) {
            return routingTable[i].prefix;
        }
    }
    return nullptr;
}

RouteEntry* findRouteByMac(const uint8_t* mac) {
    for (int i = 0; i < routingTableSize; i++) {
        if (memcmp(mac, routingTable[i].mac, 6) == 0) {
            return &routingTable[i];
        }
    }
    return nullptr;
}


bool findRouteSafe(const char *prefix, RouteEntry *&route, int &index)
{
    if (!prefix || prefix[0] == '\0') return false;

    for (int i = 0; i < routingTableSize; i++) {
        if (!routingTable[i].enabled) continue;
        if (strcmp(routingTable[i].prefix, prefix) == 0) {
            route = &routingTable[i];
            index = i;
            return true;
        }
    }

    return false;
}