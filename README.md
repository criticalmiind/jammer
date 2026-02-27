# ⚡ ShadowNet PRO — ESP8266 Wireless Research Tool

> **Created by: Shawal Ahmad Mohmand**  
> 📱 Phone: +92 304 975 8182  
> 🔍 [Search on Google](https://www.google.com/search?q=Shawal+Ahmad+Mohmand)

> ⚠️ **EDUCATIONAL USE ONLY** — This tool is for authorized cybersecurity research, wireless analysis, and defensive learning in controlled lab environments.

---

## 📋 Features

| Feature | Description |
|---------|-------------|
| **WiFi Scanner** | Scan nearby networks — shows SSID, RSSI, Channel, BSSID, Encryption |
| **Channel Occupancy** | Visual bar graph of WiFi channels 1–14 |
| **WiFi Deauth** | 802.11 deauthentication + disassociation attack with spoofed client MACs |
| **Fake APs (Beacon Flood)** | Floods ~30 fake WiFi networks with funny names |
| **Evil Twin (Phishing)** | Clones target WiFi, creates captive portal, captures passwords |
| **2.4GHz Monitor** | NRF24L01 spectrum sweep with live graph |
| **2.4GHz Jammer** | Constant, hopping, or sweep mode noise injection |
| **Packet Counter** | Promiscuous mode packet counting + deauth detection |
| **Web Dashboard** | Full control panel via phone/laptop browser |
| **Hacker Boot Animation** | Terminal-style boot sequence with loading bar |
| **About Screen** | Creator credits and contact info |

---

## 🔌 Pin Configuration — ESP8266 NodeMCU V2

### OLED Display (I2C — SH1106 / SSD1306, 128×64)
| OLED Pin | NodeMCU Pin | GPIO | Notes |
|----------|-------------|------|-------|
| VCC | 3.3V | — | Power (NOT 5V!) |
| GND | GND | — | Ground |
| SDA | D2 | GPIO 4 | I2C Data |
| SCL | D1 | GPIO 5 | I2C Clock |

> **Note:** 1.3" OLEDs use **SH1106** controller. `USE_SH1106 1` is enabled by default in `config.h`. For 0.96" SSD1306 displays, comment it out and uncomment `USE_SSD1306`.

### Navigation Buttons (4 buttons, active LOW, internal pull-up)
| Button | NodeMCU Pin | GPIO | Notes |
|--------|-------------|------|-------|
| UP | D3 | GPIO 0 | Boot pin — held HIGH at startup |
| DOWN | D4 | GPIO 2 | Built-in LED — held HIGH at startup |
| SELECT | TX | GPIO 1 | ⚠️ Do NOT use Serial Monitor |
| BACK | RX | GPIO 3 | ⚠️ Do NOT use Serial Monitor |

> **Important:** SELECT and BACK use the UART TX/RX pins. `DEBUG` is disabled by default so Serial doesn't interfere. **Close Serial Monitor** when using buttons.

### NRF24L01+ Module (SPI — 2.4GHz Monitor & Jammer)
| NRF Pin | NodeMCU Pin | GPIO | Notes |
|---------|-------------|------|-------|
| VCC | 3.3V | — | Add 10–100µF cap across VCC/GND! |
| GND | GND | — | Ground |
| CE | D0 | GPIO 16 | Chip Enable |
| CSN | D8 | GPIO 15 | SPI Chip Select |
| SCK | D5 | GPIO 14 | Hardware SPI Clock |
| MOSI | D7 | GPIO 13 | Hardware SPI MOSI |
| MISO | D6 | GPIO 12 | Hardware SPI MISO |
| IRQ | — | — | Not connected |

### Battery Voltage Divider (Optional)
| Component | NodeMCU Pin | GPIO | Notes |
|-----------|-------------|------|-------|
| VBAT → 100kΩ → junction → 100kΩ → GND | A0 | ADC0 | Divides LiPo voltage by 2 |

### TP4056 Charging Module (Optional)
| TP4056 Pin | Connection |
|------------|------------|
| IN+/IN- | USB-C power input |
| BAT+/BAT- | 3.7V LiPo battery |
| OUT+ | Through power switch → ESP8266 VIN |
| OUT- | GND |

---

## 📡 Web Dashboard

ShadowNet PRO creates a WiFi access point on boot:
- **SSID:** `ShadowNet_Panel`
- **Password:** (none/open)
- **URL:** `http://192.168.4.1`

### Features accessible via browser:
- 📡 Scan WiFi networks
- ⚔️ Launch deauth attacks
- 👻 Start beacon flood
- 🎣 Deploy evil twin phishing
- 🔑 View captured credentials
- 📊 Monitor battery, uptime, RAM, connected clients
- 🛑 Stop all attacks

Works on **any device** with a browser — phone, tablet, laptop.

---

## 🎣 Evil Twin Attack Flow

1. **Scan** → Find target WiFi network
2. **Select** → Choose network to clone
3. **Confirm** → Device creates identical open WiFi
4. **Deauth** → Continuously kicks clients off real AP
5. **Phish** → Clients connect to fake AP → captive portal asks for password
6. **Capture** → Passwords displayed on OLED + Web Dashboard

The captive portal handles Android, iOS, and Windows captive portal detection automatically.

---

## 🛠️ Compilation & Upload

### Using PlatformIO (Recommended)

1. Install [VS Code](https://code.visualstudio.com/) + [PlatformIO Extension](https://platformio.org/install/ide?install=vscode)
2. Open the `jammer/` folder in VS Code
3. Connect NodeMCU via USB
4. Click **Upload** (→ arrow) in the bottom toolbar

### Using Arduino IDE

1. Add ESP8266 board URL: `http://arduino.esp8266.com/stable/package_esp8266com_index.json`
2. Install **ESP8266** board package via Board Manager
3. Select Board: **NodeMCU 1.0 (ESP-12E Module)**
4. Install libraries:
   - Adafruit SH110X
   - Adafruit SSD1306
   - Adafruit GFX Library
   - RF24
5. Open `jammer.ino`
6. Click **Upload**

### PlatformIO Configuration (`platformio.ini`)
```ini
[platformio]
src_dir = .

[env:nodemcuv2]
platform = espressif8266
board = nodemcuv2
framework = arduino
monitor_speed = 115200
upload_speed = 921600

lib_deps =
    adafruit/Adafruit SSD1306@^2.5.7
    adafruit/Adafruit SH110X@^2.1.8
    adafruit/Adafruit GFX Library@^1.11.5
    adafruit/Adafruit BusIO@^1.14.1
    nrf24/RF24@^1.4.7
```

---

## 📁 File Structure

| File | Purpose |
|------|---------|
| `jammer.ino` | Main entry point — setup() and loop() |
| `config.h` | All configuration constants, pin definitions, branding |
| `display_manager.cpp/h` | OLED driver wrapper, splash animation, drawing helpers |
| `buttons.cpp/h` | 4-button debounced input handler |
| `menu_manager.cpp/h` | Full state-machine UI with all screens |
| `wifi_scanner.cpp/h` | WiFi network scanner |
| `wifi_attacks.cpp/h` | Deauth attack + beacon flood (fake APs) |
| `evil_twin.cpp/h` | Evil twin captive portal phishing |
| `nrf_monitor.cpp/h` | NRF24L01 spectrum analyzer |
| `packet_injection.cpp/h` | NRF24L01 2.4GHz jammer |
| `battery_manager.cpp/h` | Battery voltage monitoring |
| `web_server.cpp/h` | Web dashboard (full control panel) |
| `platformio.ini` | PlatformIO build configuration |

---

## 🔧 Troubleshooting

### Display showing garbled/sliced text
→ Your OLED uses **SH1106** controller (common on 1.3" displays)  
→ Ensure `#define USE_SH1106 1` is active in `config.h`

### Display completely blank
→ Try changing `OLED_ADDR` from `0x3C` to `0x3D` in `config.h`

### Buttons not responding
→ **Close the Serial Monitor!** TX/RX pins are shared with SELECT/BACK buttons

### WiFi deauth not working
→ The deauth now sends both deauth (0xC0) and disassociation (0xA0) frames  
→ Uses spoofed client MACs + broadcast for maximum effectiveness  
→ Burst rate is 100 frames every 50ms — very aggressive

### Web dashboard not loading
→ Connect to `ShadowNet_Panel` WiFi first  
→ Navigate to `http://192.168.4.1`

---

## 📊 Memory Usage

```
RAM:   [====      ]  43.5% (35,628 / 81,920 bytes)
Flash: [===       ]  33.5% (350,223 / 1,044,464 bytes)
```

Plenty of room for future features!

---

## 🚀 Future Plans

| Feature | Description |
|---------|-------------|
| GPS Module | Geo-tag WiFi scans (wardriving) |
| IR Blaster | Infrared remote control |
| RF Module (CC1101) | Sub-GHz signal analysis |
| SD Card Logging | Save scan results & captures |
| OTA Updates | Over-the-air firmware updates |
| PCAP Export | Wireshark-compatible packet capture |

---

## ⚖️ Legal Disclaimer

This tool is designed for **educational cybersecurity research** in **controlled lab environments only**. Unauthorized use against networks or devices you do not own is **illegal**. The creator assumes no responsibility for misuse.

---

**ShadowNet PRO v2.0** — Built with 💜 by **Shawal Ahmad Mohmand**
