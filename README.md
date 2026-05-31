# OENENet

Modular ESP-NOW + OSC network for distributed control.

## Structure
- ESP32Core: node firmware for servo and LED control
- OlimexHub: central routing hub with Ethernet/UDP bridge
- OENENetCommon: shared utilities and radio configuration

## Features
- Shared radio configuration (channel + txPower)
- Emergency window for safe pairing
- OSC message routing over ESP-NOW
