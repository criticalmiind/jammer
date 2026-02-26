# Wiring Diagram & Build Guide

## 1 — Pin Configuration Table

### OLED Display (I2C — SSD1306 / SH1106, 128×64)
| OLED Pin | ESP32 Pin | Notes |
|----------|-----------|-------|
| VCC | 3.3 V | **Do NOT use 5 V** |
| GND | GND | |
| SDA | GPIO 21 | Default I2C SDA |
| SCL | GPIO 22 | Default I2C SCL |

### Navigation Buttons (active LOW — internal pull-up)
| Button | ESP32 Pin | Notes |
|--------|-----------|-------|
| UP | GPIO 32 | INPUT_PULLUP |
| DOWN | GPIO 33 | INPUT_PULLUP |
| LEFT | GPIO 25 | INPUT_PULLUP |
| RIGHT | GPIO 26 | INPUT_PULLUP |
| SELECT | GPIO 27 | INPUT_PULLUP |
| BACK | GPIO 14 | INPUT_PULLUP |

### NRF24L01+PA+LNA Module #1 (VSPI — primary scanner)
| NRF Pin | ESP32 Pin | Notes |
|---------|-----------|-------|
| VCC | 3.3 V | Use 10 µF cap across VCC-GND |
| GND | GND | |
| CE | GPIO 4 | Chip Enable |
| CSN | GPIO 5 | SPI CS (VSPI SS) |
| SCK | GPIO 18 | VSPI CLK |
| MOSI | GPIO 23 | VSPI MOSI |
| MISO | GPIO 19 | VSPI MISO |
| IRQ | — | Not connected |

### NRF24L01+PA+LNA Module #2 (HSPI — secondary/jammer)
| NRF Pin | ESP32 Pin | Notes |
|---------|-----------|-------|
| VCC | 3.3 V | 10 µF cap |
| GND | GND | |
| CE | GPIO 16 | |
| CSN | GPIO 17 | HSPI CS |
| SCK | GPIO 14* | Shared with BACK btn — see note |
| MOSI | GPIO 13 | HSPI MOSI |
| MISO | GPIO 12 | HSPI MISO |
| IRQ | — | Not connected |

> **Note:** GPIO 14 is shared between HSPI SCK and the BACK button.  
> To avoid conflict, we remap BACK button to **GPIO 15** when using the second NRF module.  
> In the firmware `config.h`, `BTN_BACK` is set to GPIO 15.

### NRF24L01+PA+LNA Module #3 (Software SPI or secondary HSPI with different CS)
| NRF Pin | ESP32 Pin | Notes |
|---------|-----------|-------|
| VCC | 3.3 V | 10 µF cap |
| GND | GND | |
| CE | GPIO 2 | |
| CSN | GPIO 0 | Be careful — boot pin |
| SCK | GPIO 14 | Shared HSPI CLK |
| MOSI | GPIO 13 | Shared HSPI MOSI |
| MISO | GPIO 12 | Shared HSPI MISO |
| IRQ | — | Not connected |

> Module #3 shares HSPI bus with Module #2 but has a unique CSN (GPIO 0).  
> Only one of modules #2/#3 is active at any given time (CS-selected).

### Battery Voltage Divider
| Point | ESP32 Pin | Notes |
|-------|-----------|-------|
| VBAT → 100 kΩ → junction → 100 kΩ → GND | GPIO 34 (ADC1_CH6) | Divides by 2 |

### TP4056 Charging Module
| TP4056 Pin | Connection |
|------------|------------|
| IN+ / IN- | USB-C power input |
| BAT+ / BAT- | 3.7 V LiPo battery |
| OUT+ | Through power switch → ESP32 VIN/5V |
| OUT- | GND |

### Power Switch
Inline between TP4056 OUT+ and ESP32 power rail.

---

## 2 — Text-Based Wiring Diagram

```
                        ┌──────────────┐
    USB-C ──►│  TP4056     │
              │  Charger    │
              │  BAT+ BAT- │
              └──┬─────┬───┘
                 │     │
            ┌────┤     │
            │ LiPo     │
            │ 3.7V     │
            └────┤     │
                 │     │
          [SWITCH]     │
                 │     │
        ESP32 VIN    GND
                 │     │
    ┌────────────┴─────┴─────────────────┐
    │          ESP32 Wemos Lolin32       │
    │                                    │
    │  GPIO21(SDA)──►OLED SDA            │
    │  GPIO22(SCL)──►OLED SCL            │
    │                                    │
    │  GPIO4 ──►NRF1 CE                  │
    │  GPIO5 ──►NRF1 CSN                 │
    │  GPIO18──►NRF1 SCK  (VSPI)         │
    │  GPIO23──►NRF1 MOSI                │
    │  GPIO19──►NRF1 MISO                │
    │                                    │
    │  GPIO16──►NRF2 CE                  │
    │  GPIO17──►NRF2 CSN                 │
    │  GPIO14──►NRF2 SCK  (HSPI)         │
    │  GPIO13──►NRF2 MOSI                │
    │  GPIO12──►NRF2 MISO                │
    │                                    │
    │  GPIO2 ──►NRF3 CE                  │
    │  GPIO0 ──►NRF3 CSN                 │
    │  (shares HSPI bus with NRF2)       │
    │                                    │
    │  GPIO32──►BTN UP    (pull-up)      │
    │  GPIO33──►BTN DOWN  (pull-up)      │
    │  GPIO25──►BTN LEFT  (pull-up)      │
    │  GPIO26──►BTN RIGHT (pull-up)      │
    │  GPIO27──►BTN SELECT(pull-up)      │
    │  GPIO15──►BTN BACK  (pull-up)      │
    │                                    │
    │  GPIO34──►VBAT divider (ADC)       │
    └────────────────────────────────────┘
```

> **Each button:** One leg to assigned GPIO, other leg to GND.  
> Internal pull-ups are enabled in firmware.

---

## 3 — Required Libraries

| Library | Install via | Purpose |
|---------|-------------|---------|
| `Adafruit SSD1306` | Arduino Library Manager | OLED display driver |
| `Adafruit GFX Library` | Arduino Library Manager | Graphics primitives |
| `RF24` | Arduino Library Manager (TMRh20) | NRF24L01 driver |
| `ESP32 BLE Arduino` | Included with ESP32 board package | BLE scanning |
| `WiFi` | Included with ESP32 board package | WiFi scanning & raw frames |
| `esp_wifi` | ESP-IDF (included) | Promiscuous mode, deauth |

### Install ESP32 Board Package
In Arduino IDE → Preferences → Additional Board Manager URLs:
```
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
```
Then: Tools → Board Manager → search "esp32" → install **v2.x**.

---

## 4 — Compilation Instructions

1. Open `jammer.ino` in Arduino IDE (or import folder in PlatformIO).
2. Select Board: **ESP32 Dev Module**
3. Set Flash Size: **4 MB (32 Mb)**
4. Partition Scheme: **Default 4 MB with spiffs**
5. Upload Speed: **921600**
6. Select correct COM port.
7. Click **Upload** (→ button).

### PlatformIO (`platformio.ini`)
```ini
[env:esp32dev]
platform = espressif32
board = lolin32
framework = arduino
monitor_speed = 115200
upload_speed = 921600
lib_deps =
    adafruit/Adafruit SSD1306@^2.5.7
    adafruit/Adafruit GFX Library@^1.11.5
    nrf24/RF24@^1.4.7
```

---

## 5 — Memory Optimization Tips

1. **Use `F()` macro** for all string literals in `Serial.print()` and display calls — saves RAM.
2. **Avoid `String` class** — use `char[]` buffers and `snprintf()` instead.
3. **Use `PROGMEM`** for constant arrays (icon bitmaps, lookup tables).
4. **Minimize global buffers** — reuse scan-result buffers between WiFi and BLE modes.
5. **Disable Serial** in production builds (`#define DEBUG 0`) for ~1 KB RAM savings.
6. **Monitor heap**: call `ESP.getFreeHeap()` in System Info to track live usage.
7. **Use `esp_wifi_set_storage(WIFI_STORAGE_RAM)`** to avoid flash wear.

---

## 6 — Future Improvement Suggestions

| Feature | Description |
|---------|-------------|
| **SD Card Logging** | Log scan results & captures to microSD for later analysis |
| **GPS Module** | Geo-tag WiFi/BLE scans (wardriving) |
| **Buzzer / Haptic** | Audio/vibration alerts for detected threats |
| **Web Dashboard** | ESP32 hosts AP with a web UI for live data over HTTP |
| **PCAP Export** | Export captured packets in Wireshark-compatible format |
| **OTA Updates** | Over-the-air firmware updates via WiFi |
| **Rotary Encoder** | Replace UP/DOWN buttons for smoother scrolling |
| **Larger Display** | 2.4" TFT for richer graphics and color spectrum graphs |
| **Directional Antenna** | For signal direction finding |
| **Encrypted Config** | Store settings in encrypted NVS partition |
