# OENENet

OENENet is a modular ESP-NOW + OSC network designed for distributed control of servos, LEDs, and custom nodes.  
The system is composed of a central hub (OlimexHub) and multiple ESP32Core nodes, all communicating through a shared radio configuration and a lightweight OSC routing layer.

---

## Architecture

### **OlimexHub**
- Acts as the central router  
- Bridges Ethernet/UDP OSC messages to ESP-NOW  
- Manages peer discovery and routing  
- Applies shared radio configuration (channel + txPower)

### **ESP32Core**
- Node firmware for servo, LED, or custom hardware  
- Receives OSC messages via ESP-NOW  
- Sends status/feedback to the hub  
- Supports emergency window for safe pairing  
- Uses shared radio configuration

### **OENENetCommon**
Shared library containing:
- `RadioConfig` (load/save/apply radio settings)  
- Logging utilities  
- OSC helpers (based on the CNMAT OSC library)  
- Generic utilities  

This project uses the **CNMAT OSC library** for message parsing and encoding:  
https://github.com/CNMAT/OSC

---

## Features

- **Shared radio configuration** (channel + txPower) stored in NVS  
- **Emergency window** for safe pairing of new nodes  
- **OSC routing** between Ethernet and ESP-NOW  
- **Modular architecture** with a clean common library  
- **Minimal logs** for production stability  
- **Safe, deterministic workflows** for embedded systems  

---

## Repository Structure

```
/OlimexHub        → Central routing hub firmware
/ESP32Core        → Node firmware
/OENENetCommon    → Shared library (RadioConfig, Logging, Utils, OSC helpers)
```

---

## Installation

### Install the OENENetCommon library

1. Download or clone this repository.  
2. Open your Arduino `libraries` directory (its location depends on your system configuration).  
3. Copy the entire `OENENetCommon` folder into the `libraries` directory.  
4. Restart Arduino IDE so the library is detected.

After installation, the library will be available to both ESP32Core and OlimexHub sketches.

---

## Requirements

- ESP32 boards (ESP32-WROOM, ESP32 DevKit, or compatible)  
- Olimex ESP32 Gateway (for the hub)  
- Arduino IDE or PlatformIO  
- Ethernet connection for the hub  

---

## Build Instructions

1. Ensure the `OENENetCommon` library is installed.  
2. Open either `OlimexHub` or `ESP32Core` in Arduino IDE.  
3. Select the correct ESP32 board.  
4. Compile and upload.  

---

## License

This project is released under the **MIT License**.  
See the `LICENSE` file for details.
