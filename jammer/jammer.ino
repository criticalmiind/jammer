/*
 * jammer.ino — Main entry point for WRT-ESP8266
 *
 * Firmware for ESP8266 NodeMCU V2
 * Provides WiFi scanning, automated attacks (Deauth), and 2.4GHz
 * spectrum jamming for educational wireless security research.
 */

#include <Arduino.h>
#include <Wire.h>

// Core subsystems
#include "config.h"
#include "display_manager.h"
#include "buttons.h"
#include "menu_manager.h"

// Scanners & RF
#include "wifi_scanner.h"
#include "nrf_monitor.h"

// Attacks
#include "wifi_attacks.h"
#include "packet_injection.h"

// Utility
#include "battery_manager.h"
#include "web_server.h"

// ────────────────────────────────────────────────────────────────

// Splash screen timer
static unsigned long _splashStart = 0;
static bool _inSplash = true;

// ────────────────────────────────────────────────────────────────

void setup() {
#if DEBUG
  Serial.begin(115200);
  delay(10);
  DBGLN(F("\n\n"));
  DBGLN(F("========================================"));
  DBGF("[SYSTEM] Starting %s (v%s)\n", FW_NAME, FW_VERSION);
  DBGLN(F("========================================"));
#endif

  // 1. Initialize Display
  if (!display_init()) {
    DBGLN(F("[FATAL] Display initialization failed. Halting."));
    while (1) { delay(100); }
  }

  // Show splash directly
  display_splash();
  _splashStart = millis();

  // 2. Initialize Hardware / Utility
  buttons_init();
  battery_init();

  // 3. Initialize Scanners
  // (WiFi begins in Station mode but disconnected)
  wifi_scanner_init();

  // 4. Initialize NRF modules
  // We check which modules are present and set internal flags
  // These init calls also configure SPI
  bool nrf1Ok = nrf_monitor_init();
  bool nrfJamOk = packet_injection_init();

  // 5. Initialize Attacks subsystem
  wifi_attacks_init();

  // 6. Initialize App State & Menu
  menu_init();

  DBGLN(F("[SYSTEM] Boot sequence complete."));
}

void loop() {
  if (_inSplash) {
    if (millis() - _splashStart > SPLASH_DURATION_MS) {
      _inSplash = false;
      display_clear();
      display_update();
      DBGLN(F("[SYSTEM] Entering main app logic."));
    } else {
      // Pump buttons to clear any held state during boot
      buttons_poll();
      delay(10);
      return;
    }
  }

  // --- Main Application Loop ---

  // 1. Read inputs
  ButtonEvent evt = buttons_poll();

  // 2. Read sensors / utility
  battery_update();
  web_server_update();

  // 3. Service asynchronous subsystems
  packet_injection_update(); // Handles non-blocking jam transmission
  
  // 4. Service App Logic / UI
  // menu_update() controls state transitions and handles user input
  menu_update(evt);

  // 5. Render
  // _render() is called by menu_update, so we don't need a separate call here
  // display_update() might be called inside menu rendering.

  // Yield to watchdog and system tasks
  yield();
  delay(5);
}
