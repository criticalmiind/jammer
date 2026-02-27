/*
 * menu_manager.h — State-machine based menu / UI system
 * ShadowNet PRO by Shawal Ahmad Mohmand
 */

#ifndef MENU_MANAGER_H
#define MENU_MANAGER_H

#include <Arduino.h>
#include "config.h"
#include "buttons.h"

enum AppState {
  STATE_SPLASH = 0,
  STATE_MAIN_MENU,

  // WiFi Scanner
  STATE_WIFI_MENU,
  STATE_WIFI_SCANNING,
  STATE_WIFI_RESULTS,
  STATE_WIFI_DETAIL,
  STATE_WIFI_CHANNEL_OCC,

  // NRF24 Monitor
  STATE_NRF_MENU,
  STATE_NRF_SWEEP,

  // Attacks
  STATE_ATTACK_MENU,
  STATE_WIFI_DEAUTH_SELECT,
  STATE_WIFI_DEAUTH_CONFIRM,
  STATE_WIFI_DEAUTH_RUNNING,
  STATE_BEACON_FLOOD_CONFIRM,
  STATE_BEACON_FLOOD_RUNNING,
  STATE_EVIL_TWIN_SELECT,
  STATE_EVIL_TWIN_CONFIRM,
  STATE_EVIL_TWIN_RUNNING,
  STATE_NRF_JAM_MENU,
  STATE_NRF_JAM_CONFIRM,
  STATE_NRF_JAM_RUNNING,

  // Packet counter
  STATE_PACKET_COUNTER,

  // System Info
  STATE_SYSINFO,
  // About
  STATE_ABOUT,
  // Settings
  STATE_SETTINGS,

  STATE_COUNT
};

void menu_init();
void menu_update(ButtonEvent evt);
AppState menu_getState();
const char* menu_getModeLabel();

#endif // MENU_MANAGER_H
