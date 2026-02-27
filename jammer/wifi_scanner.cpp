/*
 * wifi_scanner.cpp — WiFi network scanner implementation
 *
 * Uses ESP32 WiFi library in STA mode to perform async scans.
 * Results are copied into WifiNetwork structs for UI consumption.
 */

#include "wifi_scanner.h"

static uint8_t _lastCount = 0;

/*
 * wifi_scanner_init()
 * Put WiFi into STA mode and disconnect from any AP.
 */
void wifi_scanner_init() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(50);
  DBGLN(F("[WIFI] Scanner initialized (STA mode)"));
}

/*
 * wifi_scanner_start()
 * Triggers an asynchronous scan.  Call wifi_scanner_getResults()
 * later to retrieve data.
 */
void wifi_scanner_start() {
  WiFi.scanDelete();                     // Clear previous results
  WiFi.scanNetworks(true, true);         // async=true, showHidden=true
  DBGLN(F("[WIFI] Async scan started"));
}

/*
 * wifi_scanner_getResults()
 * Copies completed scan results into the caller's array.
 * Returns the number of networks copied (0 if scan not done or failed).
 */
int wifi_scanner_getResults(WifiNetwork results[], uint8_t maxResults) {
  int16_t n = WiFi.scanComplete();
  if (n == WIFI_SCAN_RUNNING) return 0;  // Still running
  if (n < 0) {
    _lastCount = 0;
    return 0;                            // Error or not started
  }

  uint8_t count = min((uint8_t)n, maxResults);
  for (uint8_t i = 0; i < count; i++) {
    strncpy(results[i].ssid, WiFi.SSID(i).c_str(), 32);
    results[i].ssid[32] = '\0';
    results[i].rssi    = WiFi.RSSI(i);
    results[i].channel = WiFi.channel(i);
    results[i].encType = WiFi.encryptionType(i);
    const uint8_t* bssid = WiFi.BSSID(i);
    if (bssid) memcpy(results[i].bssid, bssid, 6);
  }
  _lastCount = count;

  DBGF("[WIFI] Scan complete: %d networks\n", count);
  WiFi.scanDelete();
  return count;
}

/*
 * wifi_scanner_sortByRSSI()
 * Simple insertion sort — strongest signal first.
 */
void wifi_scanner_sortByRSSI(WifiNetwork results[], uint8_t count) {
  for (uint8_t i = 1; i < count; i++) {
    WifiNetwork key = results[i];
    int8_t j = i - 1;
    while (j >= 0 && results[j].rssi < key.rssi) {
      results[j + 1] = results[j];
      j--;
    }
    results[j + 1] = key;
  }
}

/*
 * wifi_scanner_encLabel()
 * Returns a short human-readable encryption label.
 */
const char* wifi_scanner_encLabel(uint8_t encType) {
  switch (encType) {
    case ENC_TYPE_NONE: return "OPEN";
    case ENC_TYPE_WEP:  return "WEP";
    case ENC_TYPE_TKIP: return "WPA";
    case ENC_TYPE_CCMP: return "WPA2";
    case ENC_TYPE_AUTO: return "WPA*";
    default:            return "???";
  }
}

uint8_t wifi_scanner_count() {
  return _lastCount;
}

/*
 * wifi_scanner_channelOccupancy()
 * Counts how many networks are on each WiFi channel (1-14).
 */
void wifi_scanner_channelOccupancy(const WifiNetwork results[], uint8_t count, uint8_t channelCount[]) {
  memset(channelCount, 0, 15);
  for (uint8_t i = 0; i < count; i++) {
    uint8_t ch = results[i].channel;
    if (ch >= 1 && ch <= 14) {
      channelCount[ch]++;
    }
  }
}
