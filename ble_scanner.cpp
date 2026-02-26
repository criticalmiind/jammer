/*
 * ble_scanner.cpp — Bluetooth Low Energy scanner implementation
 *
 * Uses the ESP32 BLE library to perform active scans.  BLE is
 * initialized on demand and can be de-initialized to free ~60 KB
 * of heap when not needed.
 */

#include "ble_scanner.h"

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

static BLEScan* _pScan = nullptr;
static bool _initialized = false;

/*
 * ble_scanner_init()
 * Initializes BLE stack and creates a scan object.
 */
void ble_scanner_init() {
  if (_initialized) return;
  BLEDevice::init("");
  _pScan = BLEDevice::getScan();
  _pScan->setActiveScan(true);           // Active scan gets device names
  _pScan->setInterval(100);
  _pScan->setWindow(99);
  _initialized = true;
  DBGLN(F("[BLE] Scanner initialized"));
}

/*
 * ble_scanner_scan()
 * Performs a blocking BLE scan for BLE_SCAN_DURATION_S seconds.
 * Copies results into the caller's array.  Returns device count.
 */
uint8_t ble_scanner_scan(BleDevice results[], uint8_t maxResults) {
  if (!_initialized) ble_scanner_init();

  BLEScanResults* foundDevices = nullptr;

  // BLEScan::start returns BLEScanResults (library version dependent)
  #if defined(ARDUINO_ESP32_DEV) || true
    BLEScanResults scanResults = _pScan->start(BLE_SCAN_DURATION_S, false);
    foundDevices = &scanResults;
  #endif

  uint8_t count = 0;
  if (foundDevices) {
    int total = foundDevices->getCount();
    count = min((uint8_t)total, maxResults);

    for (uint8_t i = 0; i < count; i++) {
      BLEAdvertisedDevice dev = foundDevices->getDevice(i);

      // Name
      if (dev.haveName()) {
        strncpy(results[i].name, dev.getName().c_str(), 31);
      } else {
        strncpy(results[i].name, "(unknown)", 31);
      }
      results[i].name[31] = '\0';

      // MAC address
      strncpy(results[i].mac, dev.getAddress().toString().c_str(), 17);
      results[i].mac[17] = '\0';

      // RSSI
      results[i].rssi = dev.getRSSI();
    }
  }

  _pScan->clearResults();
  DBGF("[BLE] Scan complete: %d devices\n", count);
  return count;
}

/*
 * ble_scanner_deinit()
 * Shuts down BLE to reclaim ~60 KB of heap memory.
 */
void ble_scanner_deinit() {
  if (!_initialized) return;
  BLEDevice::deinit(false);
  _pScan = nullptr;
  _initialized = false;
  DBGLN(F("[BLE] De-initialized to free memory"));
}
