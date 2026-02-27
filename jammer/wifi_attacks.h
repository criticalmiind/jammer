/*
 * wifi_attacks.h — WiFi deauth, beacon flood, & interference detection
 *
 * ⚠ EDUCATIONAL USE ONLY — Controlled lab environments.
 * Created by: Shawal Ahmad Mohmand
 */

#ifndef WIFI_ATTACKS_H
#define WIFI_ATTACKS_H

#include <Arduino.h>
#include "config.h"
#include "wifi_scanner.h"

enum WifiAttackState {
  WATK_IDLE = 0,
  WATK_CONFIRMING,
  WATK_RUNNING,
  WATK_STOPPED
};

// Initialize WiFi attack subsystem
void wifi_attacks_init();

// --- Deauthentication ---
void wifi_attacks_deauth_start(const uint8_t bssid[6], uint8_t channel);
void wifi_attacks_deauth_stop();
uint16_t wifi_attacks_update();
WifiAttackState wifi_attacks_getState();
uint32_t wifi_attacks_getFrameCount();

// --- Beacon Flood (Fake APs) ---
void wifi_attacks_beacon_start();
void wifi_attacks_beacon_stop();
void wifi_attacks_beacon_update();
bool wifi_attacks_beacon_isRunning();
uint32_t wifi_attacks_beacon_getCount();

// --- Packet Counter ---
void wifi_attacks_startPacketCounter();
void wifi_attacks_stopPacketCounter();
uint32_t wifi_attacks_getPacketCount();
void wifi_attacks_resetPacketCount();
bool wifi_attacks_detectInterference();

#endif // WIFI_ATTACKS_H
