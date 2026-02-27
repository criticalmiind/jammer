/*
 * jammer.ino — ShadowNet PRO Main Entry Point
 *
 * Created by: Shawal Ahmad Mohmand
 * Contact: +92 304 975 8182
 *
 * ESP8266 NodeMCU V2 — WiFi Analysis & Wireless Security Research Tool
 */

#include <Arduino.h>
#include <Wire.h>

#include "config.h"
#include "display_manager.h"
#include "buttons.h"
#include "menu_manager.h"
#include "wifi_scanner.h"
#include "nrf_monitor.h"
#include "wifi_attacks.h"
#include "packet_injection.h"
#include "battery_manager.h"
#include "web_server.h"
#include "evil_twin.h"

static unsigned long _splashStart = 0;
static bool _inSplash = true;

void setup() {
#if DEBUG
  Serial.begin(115200);
  delay(10);
  DBGLN(F("\n\n"));
  DBGLN(F("========================================"));
  DBGF("[SYS] Starting %s v%s\n", FW_NAME, FW_VERSION);
  DBGF("[SYS] By %s\n", FW_AUTHOR);
  DBGLN(F("========================================"));
#endif

  // 1. Display
  if (!display_init()) {
    while (1) { delay(100); }
  }

  display_splash();
  _splashStart = millis();

  // 2. Hardware
  buttons_init();
  battery_init();

  // 3. Scanners
  wifi_scanner_init();

  // 4. NRF modules
  nrf_monitor_init();
  packet_injection_init();

  // 5. Attacks
  wifi_attacks_init();

  // 6. Web dashboard
  web_server_init();

  // 7. Menu
  menu_init();

  DBGLN(F("[SYS] ShadowNet PRO boot complete."));
}

void loop() {
  if (_inSplash) {
    if (millis() - _splashStart > SPLASH_DURATION_MS) {
      _inSplash = false;
      display_clear();
      display_update();
    } else {
      buttons_poll();
      delay(10);
      return;
    }
  }

  // 1. Input
  ButtonEvent evt = buttons_poll();

  // 2. Sensors
  battery_update();

  // 3. Web dashboard
  web_server_update();

  // 4. Attack services
  wifi_attacks_beacon_update();
  evil_twin_update();
  packet_injection_update();

  // 5. UI
  menu_update(evt);

  yield();
  delay(5);
}
