/*
 * wifi_scanner.h — WiFi network scanner
 */

#ifndef WIFI_SCANNER_H
#define WIFI_SCANNER_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "config.h"

// Structure holding one scanned WiFi network's info
struct WifiNetwork {
  char     ssid[33];       // 32-char max SSID + null
  int32_t  rssi;
  uint8_t  channel;
  uint8_t  encType;        // WIFI_AUTH_OPEN, WIFI_AUTH_WPA2_PSK, etc.
  uint8_t  bssid[6];
};

// Initialize WiFi in station mode for scanning
void wifi_scanner_init();

// Start an async scan. Non-blocking — returns immediately.
void wifi_scanner_start();

// Check if a scan has completed.  If yes, populates results.
// Returns the number of networks found (0 if still scanning or error).
int wifi_scanner_getResults(WifiNetwork results[], uint8_t maxResults);

// Sort results[] by RSSI (strongest first)
void wifi_scanner_sortByRSSI(WifiNetwork results[], uint8_t count);

// Get the encryption type as a short string label
const char* wifi_scanner_encLabel(uint8_t encType);

// Get the number of networks found in the last completed scan
uint8_t wifi_scanner_count();

// Channel occupancy: compute how many networks are on each channel (1-14)
// channelCount[] must be at least 15 elements.
void wifi_scanner_channelOccupancy(const WifiNetwork results[], uint8_t count, uint8_t channelCount[]);

#endif // WIFI_SCANNER_H
