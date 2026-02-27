# Jammer Project Details and Plan

## 1. Current State
- **Hardware Profile:** ESP32 with OLED (I2C), 3x NRF24L01+PA+LNA modules (SPI), TP4056 Battery Charger.
- **Current Firmware UI:** Uses 6 buttons (UP, DOWN, LEFT, RIGHT, SELECT, BACK).
- **Current Modules:** NRF1 on VSPI (Scanning), NRF2 & NRF3 on HSPI (Jamming/Injection).
- **WiFi/BLE:** Basic scanner capabilities exist.

## 2. Requirements & Planned Actions

### Task 1: 4-Button Configuration
* **Goal:** Reduce from 6 buttons to 4 (UP/RIGHT, DOWN/LEFT, SELECT, BACK/CANCEL).
* **Actions:**
  - Update `config.h` to define only 4 button pins.
  - Update `buttons.h` and `buttons.cpp` to handle mapping `BTN_UP` -> `BTN_EVT_UP` (which will also act as RIGHT in contexts where RIGHT was used), `BTN_DOWN` -> `BTN_EVT_DOWN` (acts as LEFT), `BTN_SELECT` -> `BTN_EVT_SELECT`, `BTN_BACK` -> `BTN_EVT_BACK`.
  - Refactor `menu_manager.cpp` to map existing LEFT/RIGHT logic to UP/DOWN or ignore if redundant.

### Task 2: Hardware Connection Checks & Status UI
* **Goal:** Verify OLED and 3x NRF24L01 modules. If any module is missing/failed, alert the user on both the OLED (if available) and the Web UI.
* **Actions:**
  - `display_manager.cpp`: Save `oledOk` state. If false, bypass drawing calls to prevent I2C hangs.
  - `nrf_monitor.cpp` / `packet_injection.cpp`: Provide individual boolean flags for `nrf1_ok`, `nrf2_ok`, `nrf3_ok`.
  - In `jammer.ino` `setup()`, if any hardware check fails, show a "HARDWARE ERROR" screen listing the failed modules, wait for user acknowledgment, then proceed.
  - Publish hardware failure states to the Web Dashboard.

### Task 3: Web Dashboard (ESP32 AP + HTTP Web UI)
* **Goal:** ESP32 hosts an Access Point (e.g., "Jammer_Dashboard") and serves a live web interface.
* **Actions:**
  - Create `web_server.h` and `web_server.cpp`.
  - Use `WiFi.softAP()` and `#include <WebServer.h>`.
  - Provide endpoints (`/` for UI, `/api/status` for JSON data).
  - Serve a minimal HTML page that fetches `/api/status` repeatedly to show live data (battery, device states, nrf scanning data).
  - Web UI needs to display error messages if any hardware module is missing.

### Task 4: Directional Antenna Feature
* **Goal:** Signal direction finding.
* **Actions:**
  - Repurpose or augment `nrf_monitor` to provide a "Direction Finder" mode.
  - When in this mode, it focuses on a specific channel, rapidly sampling the Carrier Detect (RPD) register and averaging the hits over a window to give an "Intensity" value (0-100%).
  - Present this on the OLED as a large bar chart and on the Web UI, which updates in real-time, helping the user sweep the directional antenna to find the signal source.

## Future AI Plans
- **Dynamic Frequency Selection (DFS) Jamming:** AI-based identification of hopping patterns to jam intelligently.
- **Protocol Classification:** Machine learning models running on the ESP32 (or edge device linked via WebUI) to identify drone vs WiFi vs BLE signatures using the NRF monitor data.
- **Automated Mapping:** Logging signal strength + GPS (if added) over time to create heatmaps on the Web UI.

## Execution Order
1. Implement Button reconfiguration (Modify config.h, buttons.x, menu_manager.cpp).
2. Enhance hardware status checks across the codebase.
3. Build the backend for the Web Dashboard (`web_server.h/cpp`).
4. Implement the API for Web Dashboard to read states.
5. Create the Directional Antenna UI mode (Direction Finder).
6. Test and refine.
