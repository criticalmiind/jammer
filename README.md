# ESP8266 WiFi Jammer & Analyzer 

## 1 — Pin Configuration Table

### OLED Display (I2C — SH1106 / SSD1306, 128×64)
| OLED Pin | NodeMCU Pin | Notes |
|----------|-------------|-------|
| VCC      | 3.3V        | Power from 3.3V pin |
| GND      | GND         | |
| SDA      | D2 (GPIO 4) | Default I2C SDA |
| SCL      | D1 (GPIO 5) | Default I2C SCL |

> **Note:** Many generic 1.3" OLEDs use the **SH1106** controller, which will look scrambled if initialized as an SSD1306. The firmware defines `USE_SH1106` in `config.h` by default to fix this.

### Navigation Buttons (active LOW — internal pull-up)
| Button | NodeMCU Pin | Notes |
|--------|-------------|-------|
| UP     | D3 (GPIO 0) | Pulled HIGH at boot |
| DOWN   | D4 (GPIO 2) | Pulled HIGH at boot |
| SELECT | TX (GPIO 1) | **Do not use Serial Monitor** |
| BACK   | RX (GPIO 3) | **Do not use Serial Monitor** |

> **Important:** To allow 4 buttons on the NodeMCU alongside the SPI and I2C buses, we repurpose the TX and RX pins. `DEBUG` is disabled by default in `config.h` so `Serial` doesn't interfere with your button presses.

### NRF24L01+ Module (SPI — 2.4 GHz Jamming)
| NRF Pin | NodeMCU Pin  | Notes |
|---------|--------------|-------|
| VCC     | 3.3V         | Add a 10µF - 100µF capacitor across VCC/GND! |
| GND     | GND          | |
| CE      | D0 (GPIO 16) | Chip Enable |
| CSN     | D8 (GPIO 15) | SPI CS |
| SCK     | D5 (GPIO 14) | Hardware SPI CLK |
| MOSI    | D7 (GPIO 13) | Hardware SPI MOSI |
| MISO    | D6 (GPIO 12) | Hardware SPI MISO |
| IRQ     | —            | Not connected |

### Battery Voltage Divider (Optional)
| Point | NodeMCU Pin | Notes |
|-------|-------------|-------|
| VBAT → 100kΩ → junction → 100kΩ → GND | A0 (ADC0) | Measures battery voltage |

---

## 2 — Required Libraries (PlatformIO)

The project leverages `platformio.ini` to handle dependencies automatically.
| Library | Purpose |
|---------|---------|
| `Adafruit SH110X` | SH1106 OLED Driver |
| `Adafruit SSD1306` | SSD1306 OLED Driver |
| `Adafruit GFX Library` | Graphics primitives |
| `RF24` | NRF24L01 Radio Driver |

---

## 3 — Compilation Instructions

### Using PlatformIO (Recommended)
1. Open the project folder in VS Code with the PlatformIO extension installed.
2. The environment is pre-configured for `nodemcuv2` (ESP8266).
3. Connect your board via USB.
4. Click the **Upload** arrow at the bottom toolbar.

### Using Arduino IDE
1. Open `jammer.ino` in the Arduino IDE.
2. Install the ESP8266 board definitions via the Boards Manager: `http://arduino.esp8266.com/stable/package_esp8266com_index.json`.
3. Select Board: **NodeMCU 1.0 (ESP-12E Module)**.
4. Select `Flash Size: 4MB (FS: 2MB OTA:~1019KB)`.
5. Open the Library Manager and install:
   - Adafruit SH110X
   - Adafruit SSD1306
   - Adafruit GFX
   - RF24
6. Click **Upload**.

---

## 4 — Features & Usage

This firmware strips away original ESP32 Bluetooth wrappers to focus solely on **WiFi and 2.4GHz RF Analysis**:
* **WiFi Scanner:** Captures SSID, RSSI, Channel, BSSID, and Encryption protocols.
* **WiFi Deauther:** Performs 802.11 deauthentication attacks against highlighted client APs.
* **NRF24 Monitor:** Sweeps channels 0-125 for 2.4 GHz frequency usage graphs and intensity.
* **NRF24 Jammer:** Spams continuous, hopping, or sweeping constant noise carriers to disrupt 2.4 GHz spectrums.
* **Web Dashboard:** Access live statistics and hardware health via a SoftAP connection (`Jammer_Dashboard`).

---

## 5 — Troubleshooting SH1106 vs SSD1306

If your OLED screen looks **sliced into vertical columns** with shifted or scrambling text, you likely have a 1.3" display. These predominantly use the **SH1106** controller chip rather than the SSD1306, which expects different framing boundaries. 

We have pre-configured `config.h` to fix this:
```cpp
#define USE_SH1106 1
```
If you are using a standard 0.96" display and the screen renders blank or incorrectly, comment out the SH1106 line and uncomment `USE_SSD1306` in `config.h`. 

**Make sure you do not open the Serial Monitor**, as doing so pulls the TX/RX pins HIGH/LOW and triggers ghost inputs on the `SELECT` and `BACK` buttons.
