/*
 * ================================================================
 *  PORTABLE WIRELESS RESEARCH TOOL
 *  Firmware for ESP32 Wemos Lolin32
 * ================================================================
 *
 *  ⚠ DISCLAIMER — EDUCATIONAL USE ONLY
 *  ─────────────────────────────────────
 *  This firmware is intended for educational cybersecurity research,
 *  wireless analysis, and defensive learning in controlled
 *  environments only.
 *
 *  Unauthorized interception, disruption, or interference with
 *  wireless communications may violate local, national, and
 *  international laws.  The author(s) assume NO liability for
 *  misuse.  Use this tool ONLY on networks and devices you own
 *  or have explicit written authorization to test.
 *
 * ================================================================
 *  FEATURES
 * ================================================================
 *  • WiFi Scanner       — SSID, RSSI, Channel, Encryption, BSSID
 *  • BLE Scanner        — Device name, MAC, RSSI, device count
 *  • 2.4 GHz Monitor    — NRF24L01 channel sweep (0-125) with graph
 *  • WiFi Deauth        — 802.11 deauthentication frames
 *  • BLE Flood          — Advertisement saturation
 *  • 2.4 GHz Jamming    — NRF24 constant / hopping / sweep modes
 *  • Packet Injection   — Custom NRF24 payloads
 *  • Packet Counter     — Promiscuous mode frame counter
 *  • Interference Alert — Deauth flood detection
 *  • Channel Occupancy  — WiFi channel analysis graph
 *  • Battery Monitor    — Voltage, percentage, low-battery warning
 *  • System Info        — Heap, uptime, firmware version
 *  • Settings           — Brightness, scan interval, deep sleep
 *
 * ================================================================
 *  HARDWARE
 * ================================================================
 *  See WIRING.md for full schematic and pin table.
 *
 * ================================================================
 *  LIBRARIES REQUIRED
 * ================================================================
 *  • Adafruit SSD1306        (OLED driver)
 *  • Adafruit GFX Library    (Graphics primitives)
 *  • RF24 by TMRh20          (NRF24L01 driver)
 *  • ESP32 BLE Arduino       (Included with ESP32 board package)
 *  • WiFi                    (Included with ESP32 board package)
 *
 * ================================================================
 *  COMPILATION
 * ================================================================
 *  Board:            ESP32 Dev Module
 *  Flash:            4 MB
 *  Partition Scheme: Default 4 MB with spiffs
 *  Upload Speed:     921600
 *
 * ================================================================
 */

#include "config.h"
#include "buttons.h"
#include "display_manager.h"
#include "battery_manager.h"
#include "wifi_scanner.h"
#include "ble_scanner.h"
#include "nrf_monitor.h"
#include "wifi_attacks.h"
#include "ble_attacks.h"
#include "packet_injection.h"
#include "menu_manager.h"

// ── Low battery warning tracking ──────────────────────────────
static unsigned long _lastBattWarn = 0;
#define BATT_WARN_INTERVAL_MS 30000   // Warn every 30 s

/*
 * setup()
 * Hardware initialization sequence:
 *   1. Serial (debug)
 *   2. OLED display → splash screen
 *   3. Buttons
 *   4. Battery ADC
 *   5. NRF24 module #1 (spectrum scanner)
 *   6. Menu system
 *
 * WiFi, BLE, and attack subsystems are initialized on demand
 * by the menu manager to conserve memory.
 */
void setup() {
  #if DEBUG
    Serial.begin(115200);
    while (!Serial && millis() < 2000);  // Wait up to 2 s for Serial
    Serial.println();
    Serial.println(F("════════════════════════════════════════════"));
    Serial.println(F("  PORTABLE WIRELESS RESEARCH TOOL"));
    Serial.println(F("  Educational Use Only"));
    Serial.println(F("════════════════════════════════════════════"));
  #endif

  // 1. Display
  if (!display_init()) {
    DBGLN(F("[SETUP] OLED FAILED — halting."));
    while (1) delay(1000);  // Can't continue without display
  }
  display_splash();

  // 2. Buttons
  buttons_init();

  // 3. Battery
  battery_init();

  // 4. NRF24 module #1 (best-effort — runs without it)
  nrf_monitor_init();

  // 5. Menu system
  menu_init();

  DBGLN(F("[SETUP] Initialization complete"));
  DBGF("[SETUP] Free heap: %lu bytes\n", (unsigned long)ESP.getFreeHeap());
}

/*
 * loop()
 * Non-blocking main loop:
 *   1. Poll buttons for input events
 *   2. Update battery readings
 *   3. Run the menu state machine (handles rendering + logic)
 *   4. Check for low battery warnings
 *   5. Yield to RTOS
 */
void loop() {
  // 1. Read button input
  ButtonEvent evt = buttons_poll();

  // 2. Update battery (internally throttled)
  battery_update();

  // 3. Menu state machine — handles everything
  menu_update(evt);

  // 4. Low battery warning overlay
  if (battery_isLow()) {
    unsigned long now = millis();
    if (now - _lastBattWarn > BATT_WARN_INTERVAL_MS) {
      _lastBattWarn = now;
      DBGLN(F("[BATT] ⚠ LOW BATTERY WARNING"));
      // Brief flash on screen
      Adafruit_SSD1306& oled = display_get();
      oled.fillRect(20, 20, 88, 24, SSD1306_BLACK);
      oled.drawRect(20, 20, 88, 24, SSD1306_WHITE);
      oled.setCursor(26, 28);
      oled.setTextColor(SSD1306_WHITE);
      oled.print(F("LOW BATTERY!"));
      oled.display();
      delay(800);  // Brief blocking display — acceptable for warning
    }
  }

  // 5. Small yield for RTOS stability
  yield();
}
