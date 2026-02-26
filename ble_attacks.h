/*
 * ble_attacks.h — Bluetooth disruption & flooding
 *
 * ⚠ EDUCATIONAL USE ONLY — Controlled lab environments.
 */

#ifndef BLE_ATTACKS_H
#define BLE_ATTACKS_H

#include <Arduino.h>
#include "config.h"

// BLE attack state
enum BleAttackState {
  BATK_IDLE = 0,
  BATK_CONFIRMING,
  BATK_RUNNING,
  BATK_STOPPED
};

// Initialize BLE attack subsystem
void ble_attacks_init();

// Start BLE advertisement flooding (spams random BLE advertisements
// to saturate nearby devices' BLE scan lists).
void ble_attacks_flood_start();

// Stop BLE flooding
void ble_attacks_flood_stop();

// Non-blocking update — call every loop()
void ble_attacks_update();

// Get current state
BleAttackState ble_attacks_getState();

// Get number of advertisement packets sent
uint32_t ble_attacks_getAdvCount();

#endif // BLE_ATTACKS_H
