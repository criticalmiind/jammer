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
| **Fake APs (Beacon Flood)** | Permanently floods 20 fake WiFi networks with funny names |
| **Evil Twin (Phishing)** | Clones target WiFi, creates captive portal, captures passwords |
| **2.4GHz Monitor** | NRF24L01 spectrum sweep with live graph |
| **2.4GHz Jammer** | Constant, hopping, or sweep mode noise injection |
| **Packet Counter** | Promiscuous mode packet counting + deauth detection |
| **Web Dashboard** | Full control panel via phone/laptop browser |
| **Mobile Lock Screen** | OLED locks its physical controls when connected remotely via phone |
| **Custom Settings** | Save custom AP Name, Password, and Channel to EEPROM memory |
| **Hacker Boot Animation** | Terminal-style boot sequence with loading bar |

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

---

## 📡 Web Dashboard & Settings

ShadowNet PRO creates a management WiFi access point on boot:
- **Default SSID:** `ShadowNet_Panel`
- **Default Password:** (none/open)
- **URL:** `http://192.168.4.1`

When you connect via your phone/laptop, you gain access to the **Remote Control Dashboard**. The physical OLED screen will show a "Mobile Using" lock screen to prevent accidental physical button presses.

### Changing the AP Name
To rename `ShadowNet_Panel` or add a password, navigate to the **Settings** tab in the Web Dashboard. You can also temporarily turn the AP on/off directly from the OLED screen in the "Settings" menu!

---

## 🎣 Evil Twin Attack Flow

1. **Scan** → Find target WiFi network
2. **Select/Clone** → Choose network to clone from OLED or Web Dashboard
3. **Confirm** → Device creates identical open WiFi and spins up a DNS server
4. **Deauth** → Background engine continuously kicks clients off the real AP
5. **Phish** → Connecting clients will be presented with a professional Fake Login portal.
   * *Pro-Tip: If the popup doesn't appear, tell the target to visit any unsecured website (e.g. `http://example.com` or `http://1.1.1.1`)*.
6. **Capture** → Passwords dynamically stream to the OLED + Web Dashboard!

The captive portal handles Android, iOS, and Windows redirection endpoints automatically.

---

## 🛠️ Compilation & Upload

### Using PlatformIO (Recommended)
1. Install [VS Code](https://code.visualstudio.com/) + [PlatformIO Extension](https://platformio.org/install/ide?install=vscode)
2. Open the `jammer/` folder 
3. Connect NodeMCU via USB
4. Click **Upload** (→ arrow) in the bottom toolbar

### Using Arduino IDE
1. Add ESP8266 board URL: `http://arduino.esp8266.com/stable/package_esp8266com_index.json`
2. Install **ESP8266** board package via Board Manager (v3.1.2)
3. Select Board: **NodeMCU 1.0 (ESP-12E Module)**
4. Install libraries: `Adafruit SH110X`, `Adafruit SSD1306`, `Adafruit GFX Library`, `RF24`
5. Open `jammer.ino` & Upload.

---

## 🔧 Troubleshooting

### Appearing offline/WiFi not found
→ Did you turn AP state to "OFF" in the device display settings? Navigate to Settings > AP State, hit Select to turn it "ON", and reboot.

### Display showing garbled/sliced text
→ Your OLED uses **SH1106** controller. Ensure `#define USE_SH1106 1` is active in `config.h`.

### Buttons not responding
→ **Close the Serial Monitor!** TX/RX pins are shared with SELECT/BACK.
→ You also cannot use physical buttons if a phone is connected to the web dashboard (Mobile Lock feature).

### Evil Twin / Phishing login page not opening
→ If your phone has mobile data enabled, it might ignore the Fake WiFi. Turn mobile data off.
→ Navigate specifically to an `http://` page (like `http://192.168.4.1` or `http://neverssl.com`), as HTTPS/HSTS domains cannot be intercepted.

### Deauth attack not kicking clients
→ Deauth requires the client and router to accept unencrypted management frames. Protected Management Frames (802.11w) ignore Deauth attacks natively.

---

## ⚖️ Legal Disclaimer

This tool is designed for **educational cybersecurity research** in **controlled lab environments only**. Unauthorized use against networks or devices you do not own is **illegal**. The creator assumes no responsibility for misuse.

---

**ShadowNet PRO v2.0** — Built with 💜 by **Shawal Ahmad Mohmand**
