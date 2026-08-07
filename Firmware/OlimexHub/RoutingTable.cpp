//RoutingTable.cpp

#include "RoutingTable.h"

// Routing table containing all known boards.
// Each entry defines:
// - prefix: short name used in OSC addressing
// - mac: MAC address of the board
// - lastAlive: timestamp of the last received /alive message

RouteEntry routingTable[] = {
   {"scheda1", {0x00, 0x11, 0x22, 0x33, 0x44, 0x55}, 0}
};

const int routingTableSize = sizeof(routingTable) / sizeof(RouteEntry);

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