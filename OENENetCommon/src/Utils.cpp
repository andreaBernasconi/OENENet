#include "Utils.h"
#include <string.h>
#include <esp_now.h>
#include <OscUtils.h>



// ---------------------------------------------------------
// ensurePeer
// ---------------------------------------------------------
bool ensurePeer(const uint8_t mac[6])
{
    if (esp_now_is_peer_exist(mac)) return true;

    esp_now_peer_info_t peer{};
    memcpy(peer.peer_addr, mac, 6);    
    peer.encrypt = false;

    return esp_now_add_peer(&peer) == ESP_OK;
}

// ---------------------------------------------------------
// sendEspNow
// ---------------------------------------------------------
bool sendEspNow(const uint8_t mac[6], const uint8_t *data, size_t len)
{
    return esp_now_send(mac, data, len) == ESP_OK;
}

// ---------------------------------------------------------
// sendOscToEspNow
// ---------------------------------------------------------
bool sendOscToEspNow(const uint8_t mac[6], OSCMessage &msg,
                     uint8_t *buffer, size_t maxLen)
{
    int len = buildOscForEspNow(msg, buffer, maxLen);
    if (len <= 0) return false;

    return sendEspNow(mac, buffer, len);
}
