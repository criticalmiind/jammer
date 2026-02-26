/*
 * ================================================================
 *  PORTABLE WIRELESS RESEARCH TOOL — CONFIGURATION
 * ================================================================
 *  This firmware is intended for educational cybersecurity research,
 *  wireless analysis, and defensive learning in controlled
 *  environments only.
 * ================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ── Firmware Version ────────────────────────────────────────────
#define FW_VERSION "1.0.0"
#define FW_NAME   "WRT-ESP32"

// ── Debug Toggle ────────────────────────────────────────────────
#define DEBUG 1
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
#define OLED_RESET      -1        // No reset pin
#define OLED_ADDR       0x3C      // I2C address (try 0x3D if blank)
#define SDA_PIN         21
#define SCL_PIN         22

// ── Navigation Buttons (active LOW, internal pull-up) ───────────
#define BTN_UP          32
#define BTN_DOWN        33
#define BTN_LEFT        25
#define BTN_RIGHT       26
#define BTN_SELECT      27
#define BTN_BACK        15        // GPIO15 (avoids HSPI conflict on 14)

#define BTN_DEBOUNCE_MS 200       // Debounce period in milliseconds

// ── NRF24L01 Module #1 — VSPI (primary scanner) ────────────────
#define NRF1_CE         4
#define NRF1_CSN        5
// VSPI defaults: SCK=18, MOSI=23, MISO=19

// ── NRF24L01 Module #2 — HSPI (secondary / jammer) ─────────────
#define NRF2_CE         16
#define NRF2_CSN        17
// HSPI: SCK=14, MOSI=13, MISO=12

// ── NRF24L01 Module #3 — HSPI (shared bus, different CS) ────────
#define NRF3_CE         2
#define NRF3_CSN        0

// ── Battery ADC ─────────────────────────────────────────────────
#define BATTERY_PIN     34        // ADC1_CH6 — input only
#define BATT_R1         100000.0  // Top resistor of divider (Ω)
#define BATT_R2         100000.0  // Bottom resistor of divider (Ω)
#define BATT_SAMPLES    16        // ADC averaging samples
#define BATT_MAX_V      4.2       // Fully charged LiPo voltage
#define BATT_MIN_V      3.3       // Cutoff voltage
#define BATT_LOW_WARN   3.5       // Low battery warning threshold
#define BATT_READ_INTERVAL_MS 5000 // How often to read battery

// ── WiFi Scanner Defaults ───────────────────────────────────────
#define WIFI_SCAN_INTERVAL_MS  5000
#define MAX_WIFI_RESULTS       20

// ── BLE Scanner Defaults ────────────────────────────────────────
#define BLE_SCAN_DURATION_S    5
#define MAX_BLE_RESULTS        20

// ── NRF24 Spectrum Sweep ────────────────────────────────────────
#define NRF_NUM_CHANNELS       126
#define NRF_SWEEP_DELAY_US     200
#define NRF_GRAPH_WIDTH        128   // Matches OLED width

// ── Menu System ─────────────────────────────────────────────────
#define MENU_MAX_ITEMS         12
#define MENU_VISIBLE_ITEMS     5     // Items visible on screen at once
#define HEADER_HEIGHT          12
#define ITEM_HEIGHT            10

// ── Display Settings ────────────────────────────────────────────
#define DEFAULT_BRIGHTNESS     255   // OLED contrast (0-255)
#define SPLASH_DURATION_MS     2000

// ── Timing ──────────────────────────────────────────────────────
#define UI_REFRESH_MS          100   // Main UI refresh rate
#define STATUS_BAR_REFRESH_MS  1000  // Status bar update rate

// ── Attack Confirmation ─────────────────────────────────────────
// When true, all offensive features require a confirmation dialog
#define REQUIRE_ATTACK_CONFIRM true

// ── Deauth Defaults ─────────────────────────────────────────────
#define DEAUTH_REASON       0x01    // Reason code: Unspecified
#define DEAUTH_FRAME_COUNT  50      // Frames per burst
#define DEAUTH_DELAY_MS     10      // Delay between frames
#define DEAUTH_BURST_INTERVAL_MS 500 // Delay between bursts

// ── NRF Jam Defaults ────────────────────────────────────────────
#define NRF_JAM_POWER       RF24_PA_MAX
#define NRF_JAM_RATE        RF24_2MBPS

#endif // CONFIG_H
