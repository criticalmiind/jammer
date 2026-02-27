/*
 * ================================================================
 *  SHADOWNET PRO — CONFIGURATION
 * ================================================================
 *  Created by: Shawal Ahmad Mohmand
 *  Contact:    +92 304 975 8182
 *  This firmware is intended for educational cybersecurity research,
 *  wireless analysis, and defensive learning in controlled
 *  environments only.
 * ================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ── Firmware Identity ───────────────────────────────────────────
#define FW_VERSION "2.0.0"
#define FW_NAME   "ShadowNet PRO"
#define FW_AUTHOR "Shawal Ahmad Mohmand"
#define FW_PHONE  "+92 304 975 8182"

// ── Debug Toggle ────────────────────────────────────────────────
#define DEBUG 0
#if DEBUG
  #define DBG(x)   Serial.print(x)
  #define DBGLN(x) Serial.println(x)
  #define DBGF(fmt, ...) Serial.printf(fmt, ##__VA_ARGS__)
#else
  #define DBG(x)
  #define DBGLN(x)
  #define DBGF(fmt, ...)
#endif

// ── OLED Display ────────────────────────────────────────────────
#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   64
#define OLED_RESET      -1
#define OLED_ADDR       0x3C
#define SDA_PIN         4         // D2
#define SCL_PIN         5         // D1

// Uncomment the specific OLED driver for your screen:
#define USE_SH1106      1         // 1.3" OLEDs (DST-013, SH1106G, etc)
// #define USE_SSD1306    1         // 0.96" OLEDs

// ── Navigation Buttons (active LOW, internal pull-up) ───────────
#define BTN_UP          0         // D3
#define BTN_DOWN        2         // D4
#define BTN_SELECT      1         // TX pin
#define BTN_BACK        3         // RX pin

#define BTN_DEBOUNCE_MS 180       // Debounce period in milliseconds

// ── NRF24L01 Module — SPI (primary scanner/jammer) ──────────────
#define NRF1_CE         16        // D0
#define NRF1_CSN        15        // D8
// SPI defaults: SCK=D5(14), MOSI=D7(13), MISO=D6(12)

// ── Battery ADC ─────────────────────────────────────────────────
#define BATTERY_PIN     A0
#define BATT_R1         100000.0
#define BATT_R2         100000.0
#define BATT_SAMPLES    16
#define BATT_MAX_V      4.2
#define BATT_MIN_V      3.3
#define BATT_LOW_WARN   3.5
#define BATT_READ_INTERVAL_MS 5000

// ── WiFi Scanner Defaults ───────────────────────────────────────
#define WIFI_SCAN_INTERVAL_MS  5000
#define MAX_WIFI_RESULTS       20

// ── NRF24 Spectrum Sweep ────────────────────────────────────────
#define NRF_NUM_CHANNELS       126
#define NRF_SWEEP_DELAY_US     200
#define NRF_GRAPH_WIDTH        128

// ── Menu System ─────────────────────────────────────────────────
#define MENU_MAX_ITEMS         12
#define MENU_VISIBLE_ITEMS     6     // More items visible with smaller font
#define HEADER_HEIGHT          10
#define ITEM_HEIGHT            9

// ── Display Settings ────────────────────────────────────────────
#define DEFAULT_BRIGHTNESS     255
#define SPLASH_DURATION_MS     4500   // Longer for boot animation

// ── Timing ──────────────────────────────────────────────────────
#define UI_REFRESH_MS          80
#define STATUS_BAR_REFRESH_MS  1000

// ── Attack Confirmation ─────────────────────────────────────────
#define REQUIRE_ATTACK_CONFIRM true

// ── Deauth Defaults ─────────────────────────────────────────────
#define DEAUTH_REASON       0x01
#define DEAUTH_FRAME_COUNT  100     // More frames per burst
#define DEAUTH_DELAY_MS     2       // Faster inter-frame gap
#define DEAUTH_BURST_INTERVAL_MS 50 // Much faster bursts

// ── Fake AP (Beacon Flood) Defaults ─────────────────────────────
#define BEACON_FRAME_COUNT  10      // Beacons per loop iteration
#define BEACON_INTERVAL_MS  100
#define MAX_FAKE_APS        30

// ── Evil Twin Defaults ──────────────────────────────────────────
#define EVIL_TWIN_CHANNEL   1
#define CAPTIVE_PORTAL_PORT 80
#define DNS_PORT            53
#define MAX_CAPTURED_CREDS  10

// ── Web Dashboard ───────────────────────────────────────────────
#define WEB_AP_SSID     "ShadowNet_Panel"
#define WEB_AP_PASS     ""              // Open AP for easy access
#define WEB_PORT        80

// ── NRF Jam Defaults ────────────────────────────────────────────
#define NRF_JAM_POWER       RF24_PA_MAX
#define NRF_JAM_RATE        RF24_2MBPS

#endif // CONFIG_H
