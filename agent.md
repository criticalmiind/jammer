# Agent – Portable Wireless Research Tool

## Project Name
Portable Wireless Research Tool

## Platform
ESP32 (Arduino Framework) — Wemos Lolin32

## Purpose
Educational Wireless Research Device covering WiFi, Bluetooth (BLE), and 2.4 GHz (NRF24L01+PA+LNA) spectrum analysis, with defensive cybersecurity features for controlled lab environments.

## Restrictions
- N/A — All features are intended for educational and authorized testing only.

## Hardware List
| # | Component | Qty | Notes |
|---|-----------|-----|-------|
| 1 | ESP32 Wemos Lolin32 | 1 | Main MCU, WiFi + BLE |
| 2 | 1.3" I2C OLED Display (128×64, SSD1306/SH1106) | 1 | UI output |
| 3 | NRF24L01+PA+LNA (2.4 GHz) | 3 | Channel sweep / jamming |
| 4 | TP4056 Type-C Charging Module | 1 | LiPo charge management |
| 5 | Push Buttons (UP, DOWN, LEFT, RIGHT, SELECT, BACK) | 6 | Navigation |
| 6 | Power Switch | 1 | On/Off |
| 7 | 3.7 V LiPo Battery | 1 | Portable power |

## Development Rules
- Clean modular code — each subsystem in its own `.cpp` / `.h` pair
- All functions commented with purpose, parameters, and return values
- Non-blocking design (`millis()`-based timing, no `delay()` in main loop)
- Memory-optimized (use `F()` macro for strings, minimize dynamic allocation)
- Configurable constants grouped at file tops or in `config.h`

## Build Settings
| Setting | Value |
|---------|-------|
| Board | ESP32 Dev Module |
| Flash | 4 MB |
| Partition Scheme | Default |
| Upload Speed | 921600 |

## File Structure
```
jammer/
├── jammer.ino            # Main entry point
├── config.h              # Global pin definitions & constants
├── display_manager.h/.cpp
├── menu_manager.h/.cpp
├── wifi_scanner.h/.cpp
├── ble_scanner.h/.cpp
├── nrf_monitor.h/.cpp
├── battery_manager.h/.cpp
├── wifi_attacks.h/.cpp
├── ble_attacks.h/.cpp
├── packet_injection.h/.cpp
├── buttons.h/.cpp
├── agent.md
├── WIRING.md
└── ESP32_Wireless_Research_Device_Full_Prompt.txt
```
