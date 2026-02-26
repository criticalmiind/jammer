/*
 * wifi_attacks.h — WiFi deauthentication & interference detection
 *
 * ⚠ EDUCATIONAL USE ONLY — Controlled lab environments.
 */

#ifndef WIFI_ATTACKS_H
#define WIFI_ATTACKS_H

#include <Arduino.h>
#include "config.h"
#include "wifi_scanner.h"

// WiFi attack state
enum WifiAttackState {
  WATK_IDLE = 0,
  WATK_CONFIRMING,      // Waiting for user confirmation
  WATK_RUNNING,         // Deauth in progress
  WATK_STOPPED
};

// Initialize WiFi attack subsystem (promiscuous mode setup)
void wifi_attacks_init();

// Start deauthentication attack on a target network.
// Requires prior call to wifi_attacks_init().
// bssid = target AP BSSID, channel = AP channel.
void wifi_attacks_deauth_start(const uint8_t bssid[6], uint8_t channel);

// Stop the running deauth attack
void wifi_attacks_deauth_stop();

// Non-blocking update — sends deauth frames in bursts.
// Call every loop(). Returns the number of frames sent this call.
uint16_t wifi_attacks_update();

// Get current state
WifiAttackState wifi_attacks_getState();

// Get total frames sent since attack started
uint32_t wifi_attacks_getFrameCount();

// Enable promiscuous mode for packet counting
void wifi_attacks_startPacketCounter();

// Stop promiscuous mode
void wifi_attacks_stopPacketCounter();

// Get total packets seen
uint32_t wifi_attacks_getPacketCount();

// Reset packet counter
void wifi_attacks_resetPacketCount();

// Detect potential deauth flood — returns true if detects
// an abnormally high rate of deauth/disassoc frames
bool wifi_attacks_detectInterference();

#endif // WIFI_ATTACKS_H
