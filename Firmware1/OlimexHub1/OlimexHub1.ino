/*
Title: Olimex Receiver for MAX → ESP-NOW Router
Board: OLIMEX ESP32-POE

Required Board Settings:
 - Flash Size:        4MB
 - Partition Scheme:  Huge APP (3MB No OTA)   [REQUIRED]
 - CPU Frequency:     240 MHz
 - PSRAM:             Disabled


*/
#include <ETH.h>
#include <WiFi.h>
#include <WiFiGeneric.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>
#include <OSCBundle.h>
#include "RoutingTable.h"
#include <Preferences.h>

Preferences prefs;

#define DEBUG 1
#if DEBUG
#define LOG(x) \
  do { Serial.print(x); } while (0)
#define LOGLN(x) \
  do { Serial.println(x); } while (0)
#else
#define LOG(x)
#define LOGLN(x)
#endif

#define ESPNOW_MAX_PAYLOAD 250

const unsigned long ALIVE_TIMEOUT_MS = 4000;

WiFiUDP Udp;

//  Ethernet Configuration (ESP32-POE)
const IPAddress ROUTER_IP(192, 168, 1, 23);
const IPAddress ROUTER_GATEWAY(192, 168, 1, 254);
const IPAddress ROUTER_SUBNET(255, 255, 255, 0);
const int LOCAL_OSC_PORT = 8888;

#define ETH_TYPE ETH_PHY_LAN8720
#define ETH_ADDR 0
#define ETH_MDC 23
#define ETH_MDIO 18
#define ETH_POWER 12
#define ETH_CLKMODE ETH_CLOCK_GPIO17_OUT


// PC
IPAddress PcIP(192, 168, 1, 5);
int PcPort = 8888;

// Radio Configuration
struct RadioConfig {
  int channel;
  int txPower;
} radioConfig;


bool onlineStatus[10] = { false };
//bool firstAliveReceived = false;


// Class BufferPrinter
class BufferPrinter : public Print {
public:
  uint8_t *buffer;
  int index;

  BufferPrinter(uint8_t *buf) {
    buffer = buf;
    index = 0;
  }

  virtual size_t write(uint8_t b) {
    buffer[index++] = b;
    return 1;
  }
};



// Load radio configuration from NVS
void loadConfig() {
  prefs.begin("radio", false);
  radioConfig.channel = prefs.getInt("channel", 6);
  radioConfig.txPower = prefs.getInt("txPower", 78);
  prefs.end();
}

// Save radio configuration to NVS
void saveConfig() {
  prefs.begin("radio", false);
  prefs.putInt("channel", radioConfig.channel);
  prefs.putInt("txPower", radioConfig.txPower);
  prefs.end();
}

// Apply the current radio configuration to Wi‑Fi hardware.
void applyConfig() {
  esp_wifi_set_channel(radioConfig.channel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_max_tx_power(radioConfig.txPower);
}

// sendOscToPC
void sendOscToPc(OSCMessage &msg) {
  Udp.beginPacket(PcIP, PcPort);
  msg.send(Udp);
  Udp.endPacket();
}
// sendStatusToMax
void sendStatusToMax() {
  for (int i = 0; i < routingTableSize; i++) {
    if (!routingTable[i].enabled) continue;

    OSCMessage msg("/alive_ack");
    msg.add(routingTable[i].prefix);
    msg.add(onlineStatus[i] ? 1 : 0);

    sendOscToPc(msg);
  }

  LOGLN("Status sent to MAX");
}
// Handle /olimex/radio/channel
void handleChannel(OSCMessage &msg, int addrOffset) {
  int newChannel = msg.getInt(0);
  radioConfig.channel = newChannel;
  saveConfig();
  applyConfig();

  OSCMessage resp("/olimex/radio/channel");
  resp.add(newChannel);
  sendOscToPc(resp);

  LOG("Channel updated to ");
  LOGLN(newChannel);
}

// Handle /olimex/radio/power
void handlePower(OSCMessage &msg, int addrOffset) {
  int newPower = msg.getInt(0);
  radioConfig.txPower = newPower;
  saveConfig();
  applyConfig();

  OSCMessage resp("/olimex/radio/power");
  resp.add(newPower);
  sendOscToPc(resp);
}
// Handle /olimex/radio/power
void handleRadioStatus(OSCMessage &msg, int addrOffset) {
  // Send current channel
  OSCMessage ch("/olimex/radio/channel");
  ch.add(radioConfig.channel);
  sendOscToPc(ch);

  // Send current TX power
  OSCMessage pw("/olimex/radio/power");
  pw.add(radioConfig.txPower);
  sendOscToPc(pw);
}

// Handle /olimex/status/request
void handleStatusRequest(OSCMessage &msg, int addrOffset) {
  sendStatusToMax();
}

// Send raw ESP‑NOW packet
bool sendEspNow(const uint8_t *mac, const uint8_t *data, size_t len) {
  esp_err_t result = esp_now_send(mac, data, len);
  if (result == ESP_OK) return true;
  return false;
}

// Handle incoming /olimex/alive messages from MAX.
void handleOlimexAlive(OSCMessage &msg, int addrOffset) {

  int aliveNumber = msg.getInt(0);

  // Forward /alive to all enabled boards
  for (int i = 0; i < routingTableSize; i++) {
    if (!routingTable[i].enabled) continue;
    OSCMessage alive("/alive");
    static uint8_t outBuffer[ESPNOW_MAX_PAYLOAD];
    BufferPrinter bp(outBuffer);
    alive.send(bp);
    int outLen = bp.index;
    while (outLen % 4 != 0) outBuffer[outLen++] = 0;
    sendEspNow(routingTable[i].mac, outBuffer, outLen);
  }
  // Send alive_ack back to MAX
  OSCMessage reply("/alive_ack");
  reply.add("olimex").add(aliveNumber);
  sendOscToPc(reply);
}

// ESP-NOW receive callback
void onEspNowRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {

  static uint8_t temp[ESPNOW_MAX_PAYLOAD];
  memcpy(temp, data, len);

  OSCMessage inMsg;
  inMsg.fill(temp, len);

  if (inMsg.hasError()) {
    LOGLN("Error decoding OSC message");
    return;
  }

  const char *prefix = findPrefixByMac(info->src_addr);
  if (!prefix) {
    LOGLN("Unknown MAC: cannot reconstruct OSC address");
    return;
  }
  // Extract OSC address from incoming message
  char cmd[64];
  inMsg.getAddress(cmd);

  // --- Alive handling ---
  if (strcmp(cmd, "/alive_ack") == 0) {
    RouteEntry *route = findRouteByMac(info->src_addr);
    if (route) {
      route->lastAlive = millis();
    }
    return;
  }

  
  OSCMessage outMsg(cmd); 
  outMsg.add(prefix);


  // Copy all arguments from the incoming message
  for (int i = 0; i < inMsg.size(); i++) {
    if (inMsg.isInt(i)) outMsg.add(inMsg.getInt(i));
    else if (inMsg.isFloat(i)) outMsg.add(inMsg.getFloat(i));
    else if (inMsg.isString(i)) {
      char s[64];
      inMsg.getString(i, s, 64);
      outMsg.add(s);
    }
  }
  sendOscToPc(outMsg);
}

// Check alive timeout for all boards
void checkAliveTimeout() {
  static unsigned long lastAliveCheck = 0;
  unsigned long now = millis();

  if (now - lastAliveCheck < 1000) return;
  lastAliveCheck = now;

  for (int i = 0; i < routingTableSize; i++) {
    if (!routingTable[i].enabled) continue;

    bool online = routingTable[i].lastAlive > 0 && (now - routingTable[i].lastAlive) < ALIVE_TIMEOUT_MS;

    if (online != onlineStatus[i]) {
      onlineStatus[i] = online;

      // Send unified alive ack message
      OSCMessage msg("/alive_ack");
      msg.add(routingTable[i].prefix);
      msg.add(online ? 1 : 0);
      sendOscToPc(msg);

      LOG(routingTable[i].prefix);
      LOG(online ? " → ONLINE" : " → OFFLINE");
      LOGLN("");
    }
  }
}

// Ethernet event handler
void onEthEvent(arduino_event_id_t event) {
  switch (event) {

    case ARDUINO_EVENT_ETH_DISCONNECTED:
      LOGLN("Ethernet disconnected");
      break;

    case ARDUINO_EVENT_ETH_CONNECTED:
      LOGLN("Ethernet link up");
      Udp.begin(LOCAL_OSC_PORT);
      sendStatusToMax();
      break;

    case ARDUINO_EVENT_ETH_GOT_IP:
      LOGLN("Ethernet reconnected");
      break;

    default:
      break;
  }
}




// Setup
void setup() {
  Serial.begin(115200);
  delay(200);

  loadConfig();

  Network.onEvent(onEthEvent);

  ETH.begin(ETH_TYPE, ETH_ADDR, ETH_MDC, ETH_MDIO, ETH_POWER, ETH_CLKMODE);
  ETH.config(ROUTER_IP, ROUTER_GATEWAY, ROUTER_SUBNET);

  LOGLN("Waiting for Ethernet link...");

  unsigned long ethStart = millis();
  while (!ETH.linkUp() && millis() - ethStart < 10000) {
    LOGLN(".");
    delay(300);
  }
  if (!ETH.linkUp()) {
    LOGLN("Ethernet timeout - continuing without link");
  } else {
    LOGLN("Ethernet OK");
  }

  WiFi.mode(WIFI_STA);

  applyConfig();

  if (esp_now_init() != ESP_OK) {
    LOGLN("ESP-NOW initialization error");
    return;
  }


  esp_now_register_recv_cb(onEspNowRecv);


  for (int i = 0; i < routingTableSize; i++) {
    if (!routingTable[i].enabled) continue;
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, routingTable[i].mac, 6);
    peer.channel = 0;
    peer.encrypt = false;

    if (esp_now_add_peer(&peer) == ESP_OK) {
      LOG("Peer added:");
      LOGLN(routingTable[i].prefix);
    } else {
      LOG("Peer add error: ");
      LOGLN(routingTable[i].prefix);
    }
  }

  LOGLN("Router ready");

  sendStatusToMax();
};

// Loop
void loop() {
  checkAliveTimeout();

  int packetSize = Udp.parsePacket();
  if (packetSize <= 0) return;

  uint8_t buffer[512];
  int len = Udp.read(buffer, sizeof(buffer));

  OSCBundle bundle;
  bundle.fill(buffer, len);

  if (bundle.hasError()) {
    LOGLN("Errore OSC in ingresso");
    return;
  }

  for (int i = 0; i < bundle.size(); i++) {
    OSCMessage msg = bundle.getOSCMessage(i);

    char address[256];
    msg.getAddress(address);


    if (msg.route("/olimex/radio/channel", handleChannel)) continue;
    if (msg.route("/olimex/radio/power", handlePower)) continue;
    if (msg.route("/olimex/status/request", handleStatusRequest)) continue;
    if (msg.route("/olimex/alive", handleOlimexAlive)) continue;
    if (msg.route("/olimex/radio/status/request", handleRadioStatus)) continue;


    if (address[0] != '/') {
      LOG("Invalid OSC address: ");
      LOGLN(address);
      continue;
    }

    const char *addr = address + 1;
    const char *slash = strchr(addr, '/');
    if (!slash) {
      LOG("OSC address missing command: ");
      LOGLN(address);
      continue;
    }

    char prefix[32];
    int prefixLen = slash - addr;
    strncpy(prefix, addr, prefixLen);
    prefix[prefixLen] = '\0';

    char command[64];
    strncpy(command, slash, sizeof(command));
    command[sizeof(command) - 1] = '\0';

    RouteEntry *route = findRoute(prefix);
    if (!route) {
      LOG("Unknown prefix: ");
      LOGLN(prefix);
      continue;
    }

    OSCMessage outMsg(command);

    for (int j = 0; j < msg.size(); j++) {
      if (msg.isInt(j)) outMsg.add(msg.getInt(j));
      else if (msg.isFloat(j)) outMsg.add(msg.getFloat(j));
      else if (msg.isString(j)) {
        char s[64];
        msg.getString(j, s, 64);
        outMsg.add(s);
      }
    }

    static uint8_t outBuffer[ESPNOW_MAX_PAYLOAD];
    BufferPrinter bp(outBuffer);
    outMsg.send(bp);
    int outLen = bp.index;

    while (outLen % 4 != 0) outBuffer[outLen++] = 0;

    if (outLen > ESPNOW_MAX_PAYLOAD) {
      LOG("OSC message too large: ");
      LOG(outLen);
      LOGLN(" - send cancelled");
      continue;
    }


    // ← aggiunta: controlla se la scheda è online
    if (!onlineStatus[route - routingTable]) {
      LOG("Board offline, message discarded: ");
      LOGLN(route->prefix);
      continue;
    }
    sendEspNow(route->mac, outBuffer, outLen);
  }
}
