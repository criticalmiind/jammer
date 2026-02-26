/*
 * menu_manager.h — State-machine based menu / UI system
 */

#ifndef MENU_MANAGER_H
#define MENU_MANAGER_H

#include <Arduino.h>
#include "config.h"
#include "buttons.h"

// ── Application States (screens) ────────────────────────────────
enum AppState {
  STATE_SPLASH = 0,
  STATE_MAIN_MENU,

  // WiFi Scanner sub-states
  STATE_WIFI_MENU,
  STATE_WIFI_SCANNING,
  STATE_WIFI_RESULTS,
  STATE_WIFI_DETAIL,
  STATE_WIFI_CHANNEL_OCC,

  // BLE Scanner sub-states
  STATE_BLE_MENU,
  STATE_BLE_SCANNING,
  STATE_BLE_RESULTS,

  // NRF24 Monitor sub-states
  STATE_NRF_MENU,
  STATE_NRF_SWEEP,

  // Attack sub-states
  STATE_ATTACK_MENU,
  STATE_WIFI_DEAUTH_SELECT,
  STATE_WIFI_DEAUTH_CONFIRM,
  STATE_WIFI_DEAUTH_RUNNING,
  STATE_BLE_FLOOD_CONFIRM,
  STATE_BLE_FLOOD_RUNNING,
  STATE_NRF_JAM_MENU,
  STATE_NRF_JAM_CONFIRM,
  STATE_NRF_JAM_RUNNING,

  // Packet counter
  STATE_PACKET_COUNTER,

  // System Info
  STATE_SYSINFO,

  // Settings
  STATE_SETTINGS,

  STATE_COUNT  // sentinel
};

// Initialize the menu system
void menu_init();

// Main update function — call every loop().
// Handles button input, state transitions, and screen rendering.
void menu_update(ButtonEvent evt);

// Get the current state
AppState menu_getState();

// Get a mode label string for the status bar
const char* menu_getModeLabel();

#endif // MENU_MANAGER_H
