/*
 * ble_scanner.h — Bluetooth Low Energy scanner
 */

#ifndef BLE_SCANNER_H
#define BLE_SCANNER_H

#include <Arduino.h>
#include "config.h"

// Structure holding one scanned BLE device
struct BleDevice {
  char    name[32];
  char    mac[18];      // "AA:BB:CC:DD:EE:FF"
  int     rssi;
};

// Initialize the BLE stack for scanning
void ble_scanner_init();

// Start a BLE scan (blocking for BLE_SCAN_DURATION_S seconds).
// Returns the number of devices found.
uint8_t ble_scanner_scan(BleDevice results[], uint8_t maxResults);

// De-initialize BLE to free memory when not in use
void ble_scanner_deinit();

#endif // BLE_SCANNER_H
