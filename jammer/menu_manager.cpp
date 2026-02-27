/*
 * menu_manager.cpp — ShadowNet PRO UI State Machine
 *
 * ================================================================
 *  Created by: Shawal Ahmad Mohmand
 *  Educational cybersecurity research tool.
 * ================================================================
 */

#include "menu_manager.h"
#include "display_manager.h"
#include "wifi_scanner.h"
#include "nrf_monitor.h"
#include "wifi_attacks.h"
#include "packet_injection.h"
#include "battery_manager.h"
#include "evil_twin.h"

// ── State ────────────────────────────────────────────────────────
static AppState _state = STATE_SPLASH;
static uint8_t  _cursor = 0;
static uint8_t  _scroll = 0;
static unsigned long _stateEnterTime = 0;
static unsigned long _lastRender = 0;

// ── Scan caches ─────────────────────────────────────────────────
static WifiNetwork _wifiResults[MAX_WIFI_RESULTS];
static uint8_t     _wifiCount = 0;
static bool        _wifiScanActive = false;

// ── NRF ─────────────────────────────────────────────────────────
static uint8_t _nrfSpectrum[NRF_NUM_CHANNELS];

// ── Settings ────────────────────────────────────────────────────
static uint8_t _brightness = DEFAULT_BRIGHTNESS;
static uint16_t _scanInterval = WIFI_SCAN_INTERVAL_MS;

// ── NRF jam params ──────────────────────────────────────────────
static uint8_t _jamStartCh = 0;
static uint8_t _jamEndCh   = 125;
static NrfJamMode _jamMode = NJAM_MODE_HOPPING;

// ── Saved cursors ───────────────────────────────────────────────
static uint8_t _savedDeauthCursor = 0;
static uint8_t _savedETCursor = 0;

// ── Helpers ─────────────────────────────────────────────────────
static void _setState(AppState s) {
  _state = s;
  _cursor = 0;
  _scroll = 0;
  _stateEnterTime = millis();
}

static void _setCursorBounds(uint8_t maxIdx) {
  if (_cursor > maxIdx) _cursor = maxIdx;
  if (_cursor < _scroll) _scroll = _cursor;
  if (_cursor >= _scroll + MENU_VISIBLE_ITEMS) _scroll = _cursor - MENU_VISIBLE_ITEMS + 1;
}

// ═════════════════════════════════════════════════════════════════
//  RENDER FUNCTIONS
// ═════════════════════════════════════════════════════════════════

// ── Main Menu ───────────────────────────────────────────────────
static const char* _mainItems[] = {
  "WiFi Scanner",
  "2.4GHz Monitor",
  "Attacks",
  "Packet Counter",
  "System Info",
  "About",
  "Settings"
};
static const uint8_t _mainCount = 7;

static void _renderMainMenu() {
  display_statusBar(battery_getPercent(), "MENU");
  display_list(_mainItems, _mainCount, _cursor, _scroll);
}

// ── WiFi Menu ───────────────────────────────────────────────────
static const char* _wifiMenuItems[] = { "Scan Networks", "Channel Occupancy", "Back" };
static const uint8_t _wifiMenuCount = 3;

static void _renderWifiMenu() {
  display_statusBar(battery_getPercent(), "WiFi");
  display_list(_wifiMenuItems, _wifiMenuCount, _cursor, _scroll);
}

// ── WiFi Scanning ───────────────────────────────────────────────
static void _renderWifiScanning() {
  display_progress("WiFi Scanning...", 0);
}

// ── WiFi Results ────────────────────────────────────────────────
static char _wifiLabels[MAX_WIFI_RESULTS][22];
static const char* _wifiLabelPtrs[MAX_WIFI_RESULTS];

static void _renderWifiResults() {
  display_statusBar(battery_getPercent(), "WiFi");
  if (_wifiCount == 0) {
    display_centerText("No networks found", 30, 1);
    return;
  }
  for (uint8_t i = 0; i < _wifiCount; i++) {
    snprintf(_wifiLabels[i], 22, "%-12.12s %ddBm",
             _wifiResults[i].ssid, _wifiResults[i].rssi);
    _wifiLabelPtrs[i] = _wifiLabels[i];
  }
  display_list(_wifiLabelPtrs, _wifiCount, _cursor, _scroll);
}

// ── WiFi Detail ─────────────────────────────────────────────────
static void _renderWifiDetail() {
  if (_cursor >= _wifiCount) return;
  WifiNetwork& n = _wifiResults[_cursor];
  display_statusBar(battery_getPercent(), "Detail");
  OLED_CLASS& oled = display_get();
  int16_t y = HEADER_HEIGHT + 2;
  oled.setCursor(0, y); oled.print(F("SSID: ")); oled.print(n.ssid); y += 9;
  oled.setCursor(0, y);
  oled.printf("BSSID:%02X:%02X:%02X:%02X:%02X:%02X", n.bssid[0], n.bssid[1],
              n.bssid[2], n.bssid[3], n.bssid[4], n.bssid[5]); y += 9;
  oled.setCursor(0, y); oled.print(F("RSSI: ")); oled.print(n.rssi); oled.print(F(" dBm")); y += 9;
  oled.setCursor(0, y); oled.print(F("Ch: ")); oled.print(n.channel);
  oled.print(F("  Enc: ")); oled.print(wifi_scanner_encLabel(n.encType)); y += 9;
  uint8_t pct = map(constrain(n.rssi, -100, -30), -100, -30, 0, 100);
  display_barGraph(0, y, 128, 7, pct);
}

// ── Channel Occupancy ───────────────────────────────────────────
static void _renderWifiChannelOcc() {
  display_statusBar(battery_getPercent(), "ChOcc");
  OLED_CLASS& oled = display_get();
  uint8_t chCount[15];
  wifi_scanner_channelOccupancy(_wifiResults, _wifiCount, chCount);
  uint8_t maxC = 1;
  for (uint8_t i = 1; i <= 14; i++) if (chCount[i] > maxC) maxC = chCount[i];
  int16_t barW = 8, gap = 1, startX = 2, bottom = 56, maxH = 36;
  for (uint8_t i = 1; i <= 14; i++) {
    int16_t x = startX + (i - 1) * (barW + gap);
    int16_t h = map(chCount[i], 0, maxC, 0, maxH);
    if (h > 0) oled.fillRect(x, bottom - h, barW, h, OLED_COLOR_WHITE);
    oled.setCursor(x + 1, 58);
    if (i < 10) oled.print(i);
    else { oled.setCursor(x - 1, 58); oled.print(i); }
  }
}

// ── NRF Menu ────────────────────────────────────────────────────
static const char* _nrfMenuItems[] = { "Channel Sweep", "Back" };
static const uint8_t _nrfMenuCount = 2;

static void _renderNrfMenu() {
  display_statusBar(battery_getPercent(), "NRF24");
  display_list(_nrfMenuItems, _nrfMenuCount, _cursor, _scroll);
}

// ── NRF Sweep ───────────────────────────────────────────────────
static void _renderNrfSweep() {
  display_statusBar(battery_getPercent(), "2.4GHz");
  OLED_CLASS& oled = display_get();
  int16_t bottom = 56, maxH = 38;
  uint8_t peak = nrf_monitor_peakChannel(_nrfSpectrum);
  for (uint8_t i = 0; i < NRF_NUM_CHANNELS; i++) {
    int16_t x = map(i, 0, NRF_NUM_CHANNELS, 0, SCREEN_WIDTH);
    display_vertBar(x, bottom, maxH, _nrfSpectrum[i], 10);
  }
  char peakBuf[24];
  snprintf(peakBuf, 24, "Peak: ch%d (%dMHz)", peak, 2400 + peak);
  oled.setCursor(0, 58); oled.print(peakBuf);
}

// ── Attack Menu ─────────────────────────────────────────────────
static const char* _attackMenuItems[] = {
  "WiFi Deauth",
  "Fake APs (Beacon)",
  "Evil Twin (Phish)",
  "2.4GHz Jam",
  "Back"
};
static const uint8_t _attackMenuCount = 5;

static void _renderAttackMenu() {
  display_statusBar(battery_getPercent(), "ATTACK");
  display_list(_attackMenuItems, _attackMenuCount, _cursor, _scroll);
}

// ── Deauth Select ───────────────────────────────────────────────
static void _renderDeauthSelect() {
  display_statusBar(battery_getPercent(), "DEAUTH");
  if (_wifiCount == 0) {
    display_centerText("No targets!", 24, 1);
    display_centerText("Scan WiFi first", 36, 1);
    return;
  }
  for (uint8_t i = 0; i < _wifiCount; i++) {
    snprintf(_wifiLabels[i], 22, "%-12.12s ch%d", _wifiResults[i].ssid, _wifiResults[i].channel);
    _wifiLabelPtrs[i] = _wifiLabels[i];
  }
  display_list(_wifiLabelPtrs, _wifiCount, _cursor, _scroll);
}

// ── Deauth Confirm ──────────────────────────────────────────────
static void _renderDeauthConfirm() {
  display_confirmDialog("! DEAUTH !", "Educational Only!", "Is this YOUR device?");
}

// ── Deauth Running ──────────────────────────────────────────────
static void _renderDeauthRunning() {
  display_statusBar(battery_getPercent(), "DEAUTH");
  OLED_CLASS& oled = display_get();
  oled.setCursor(0, HEADER_HEIGHT + 3);
  oled.print(F("Target: "));
  if (_savedDeauthCursor < _wifiCount) oled.print(_wifiResults[_savedDeauthCursor].ssid);
  oled.setCursor(0, HEADER_HEIGHT + 14);
  oled.print(F("Frames: ")); oled.print(wifi_attacks_getFrameCount());
  oled.setCursor(0, HEADER_HEIGHT + 25);
  oled.print(F("Status: ACTIVE"));
  oled.setCursor(0, HEADER_HEIGHT + 40);
  oled.print(F("[BACK] to stop"));
}

// ── Beacon Flood Confirm ────────────────────────────────────────
static void _renderBeaconConfirm() {
  display_confirmDialog("! FAKE APs !", "Floods 30 fake SSIDs", "Educational only!");
}

// ── Beacon Flood Running ────────────────────────────────────────
static void _renderBeaconRunning() {
  display_statusBar(battery_getPercent(), "BEACON");
  OLED_CLASS& oled = display_get();
  oled.setCursor(0, HEADER_HEIGHT + 3);
  oled.print(F("Beacon Flood Active"));
  oled.setCursor(0, HEADER_HEIGHT + 14);
  oled.print(F("Beacons: ")); oled.print(wifi_attacks_beacon_getCount());
  oled.setCursor(0, HEADER_HEIGHT + 25);
  oled.print(F("Fake APs: 30"));
  oled.setCursor(0, HEADER_HEIGHT + 40);
  oled.print(F("[BACK] to stop"));
}

// ── Evil Twin Select ────────────────────────────────────────────
static void _renderETSelect() {
  display_statusBar(battery_getPercent(), "E.TWIN");
  if (_wifiCount == 0) {
    display_centerText("No targets!", 24, 1);
    display_centerText("Scan WiFi first", 36, 1);
    return;
  }
  for (uint8_t i = 0; i < _wifiCount; i++) {
    snprintf(_wifiLabels[i], 22, "%-12.12s ch%d", _wifiResults[i].ssid, _wifiResults[i].channel);
    _wifiLabelPtrs[i] = _wifiLabels[i];
  }
  display_list(_wifiLabelPtrs, _wifiCount, _cursor, _scroll);
}

// ── Evil Twin Confirm ───────────────────────────────────────────
static void _renderETConfirm() {
  display_confirmDialog("! EVIL TWIN !", "Clone WiFi + Phish", "Is this YOUR AP?");
}

// ── Evil Twin Running ───────────────────────────────────────────
static void _renderETRunning() {
  display_statusBar(battery_getPercent(), "PHISH");
  OLED_CLASS& oled = display_get();
  oled.setCursor(0, HEADER_HEIGHT + 2);
  oled.print(F("Evil Twin Active"));
  oled.setCursor(0, HEADER_HEIGHT + 12);
  oled.print(F("SSID: "));
  if (_savedETCursor < _wifiCount) {
    // Truncate to fit
    char buf[16];
    snprintf(buf, 16, "%.14s", _wifiResults[_savedETCursor].ssid);
    oled.print(buf);
  }
  oled.setCursor(0, HEADER_HEIGHT + 22);
  oled.print(F("IP: ")); oled.print(evil_twin_getIP());
  oled.setCursor(0, HEADER_HEIGHT + 32);
  oled.print(F("Creds: ")); oled.print(evil_twin_getCredCount());
  oled.print(F(" | Deauth: ")); oled.print(evil_twin_getDeauthCount());
  // Show last captured password
  if (evil_twin_getCredCount() > 0) {
    CapturedCred* c = evil_twin_getCreds();
    oled.setCursor(0, HEADER_HEIGHT + 42);
    oled.print(F("PW: "));
    char pw[18];
    snprintf(pw, 18, "%.16s", c[evil_twin_getCredCount()-1].password);
    oled.print(pw);
  } else {
    oled.setCursor(0, HEADER_HEIGHT + 42);
    oled.print(F("[BACK] to stop"));
  }
}

// ── NRF Jam Menu ────────────────────────────────────────────────
static const char* _jamModeItems[] = { "Constant (1ch)", "Hopping (rand)", "Sweep (all)", "Back" };
static const uint8_t _jamModeCount = 4;

static void _renderNrfJamMenu() {
  display_statusBar(battery_getPercent(), "NRF JAM");
  display_list(_jamModeItems, _jamModeCount, _cursor, _scroll);
}

static void _renderNrfJamConfirm() {
  display_confirmDialog("! 2.4GHz JAM !", "Educational Only!", "Is this YOUR network?");
}

static void _renderNrfJamRunning() {
  display_statusBar(battery_getPercent(), "JAMMING");
  OLED_CLASS& oled = display_get();
  const char* modeStr = "???";
  switch (_jamMode) {
    case NJAM_MODE_CONSTANT: modeStr = "Constant"; break;
    case NJAM_MODE_HOPPING:  modeStr = "Hopping";  break;
    case NJAM_MODE_SWEEP:    modeStr = "Sweep";    break;
  }
  oled.setCursor(0, HEADER_HEIGHT + 3);
  oled.print(F("Mode: ")); oled.print(modeStr);
  oled.setCursor(0, HEADER_HEIGHT + 14);
  oled.print(F("Ch: ")); oled.print(_jamStartCh); oled.print(F("-")); oled.print(_jamEndCh);
  oled.setCursor(0, HEADER_HEIGHT + 25);
  oled.print(F("TX: ")); oled.print(packet_injection_getTxCount());
  oled.setCursor(0, HEADER_HEIGHT + 40);
  oled.print(F("[BACK] to stop"));
}

// ── Packet Counter ──────────────────────────────────────────────
static void _renderPacketCounter() {
  display_statusBar(battery_getPercent(), "PKTS");
  OLED_CLASS& oled = display_get();
  oled.setCursor(0, HEADER_HEIGHT + 3);
  oled.print(F("Packet Counter"));
  oled.setCursor(0, HEADER_HEIGHT + 16);
  oled.print(F("Total: ")); oled.print(wifi_attacks_getPacketCount());
  bool intf = wifi_attacks_detectInterference();
  oled.setCursor(0, HEADER_HEIGHT + 28);
  oled.print(F("Deauth Alert: ")); oled.print(intf ? "YES!" : "No");
  oled.setCursor(0, HEADER_HEIGHT + 42);
  oled.print(F("[SEL]Reset [BACK]Exit"));
}

// ── System Info ─────────────────────────────────────────────────
static void _renderSysInfo() {
  display_statusBar(battery_getPercent(), "INFO");
  OLED_CLASS& oled = display_get();
  int16_t y = HEADER_HEIGHT + 2;
  oled.setCursor(0, y);
  oled.printf("Batt: %.1fV (%d%%)", battery_getVoltage(), battery_getPercent()); y += 9;
  oled.setCursor(0, y);
  oled.printf("Heap: %lu B", (unsigned long)ESP.getFreeHeap()); y += 9;
  oled.setCursor(0, y);
  oled.print(F("FW: ")); oled.print(FW_NAME); y += 9;
  oled.setCursor(0, y);
  oled.print(F("v")); oled.print(FW_VERSION); y += 9;
  unsigned long up = millis() / 1000;
  oled.setCursor(0, y);
  oled.printf("Up: %luh %lum %lus", up / 3600, (up % 3600) / 60, up % 60); y += 9;
  oled.setCursor(0, y);
  oled.print(F("NRF: ")); oled.print(nrf_monitor_isConnected() ? "OK" : "N/A");
}

// ── About Screen ────────────────────────────────────────────────
static void _renderAbout() {
  OLED_CLASS& oled = display_get();
  oled.setCursor(10, 0);
  oled.print(F(">> ShadowNet PRO <<"));

  for (int x = 6; x < 122; x += 2) oled.drawPixel(x, 10, OLED_COLOR_WHITE);

  oled.setCursor(0, 14);
  oled.print(F("Created by:"));
  oled.setCursor(0, 24);
  oled.print(F("Shawal Ahmad Mohmand"));
  oled.setCursor(0, 34);
  oled.print(F("Phone:"));
  oled.setCursor(0, 43);
  oled.print(F("+92 304 975 8182"));

  for (int x = 6; x < 122; x += 2) oled.drawPixel(x, 52, OLED_COLOR_WHITE);

  oled.setCursor(6, 55);
  oled.print(F("Search Google for me"));
}

// ── Settings ────────────────────────────────────────────────────
static void _renderSettings() {
  display_statusBar(battery_getPercent(), "SET");
  OLED_CLASS& oled = display_get();
  int16_t y = HEADER_HEIGHT + 2;

  const char* settLabels[] = {"Brightness", "Scan Intv", "Deep Sleep", "Back"};
  char settVals[4][16];
  snprintf(settVals[0], 16, "%d", _brightness);
  snprintf(settVals[1], 16, "%ds", _scanInterval / 1000);
  snprintf(settVals[2], 16, "Now");
  snprintf(settVals[3], 16, "");

  for (uint8_t i = 0; i < 4; i++) {
    bool sel = (_cursor == i);
    if (sel) { oled.fillRect(0, y - 1, 128, ITEM_HEIGHT, OLED_COLOR_WHITE); oled.setTextColor(OLED_COLOR_BLACK); }
    oled.setCursor(4, y);
    oled.print(settLabels[i]);
    if (strlen(settVals[i]) > 0) { oled.print(F(": ")); oled.print(settVals[i]); }
    oled.setTextColor(OLED_COLOR_WHITE);
    y += ITEM_HEIGHT;
  }
}

// ═════════════════════════════════════════════════════════════════
//  INPUT HANDLERS
// ═════════════════════════════════════════════════════════════════

static void _handleMainMenu(ButtonEvent evt) {
  switch (evt) {
    case BTN_EVT_UP:    if (_cursor > 0) _cursor--; break;
    case BTN_EVT_DOWN:  if (_cursor < _mainCount - 1) _cursor++; break;
    case BTN_EVT_SELECT:
      switch (_cursor) {
        case 0: _setState(STATE_WIFI_MENU); break;
        case 1: _setState(STATE_NRF_MENU); break;
        case 2: _setState(STATE_ATTACK_MENU); break;
        case 3: wifi_attacks_startPacketCounter(); _setState(STATE_PACKET_COUNTER); break;
        case 4: _setState(STATE_SYSINFO); break;
        case 5: _setState(STATE_ABOUT); break;
        case 6: _setState(STATE_SETTINGS); break;
      }
      break;
    default: break;
  }
  _setCursorBounds(_mainCount - 1);
}

static void _handleWifiMenu(ButtonEvent evt) {
  switch (evt) {
    case BTN_EVT_UP:    if (_cursor > 0) _cursor--; break;
    case BTN_EVT_DOWN:  if (_cursor < _wifiMenuCount - 1) _cursor++; break;
    case BTN_EVT_BACK:  _setState(STATE_MAIN_MENU); return;
    case BTN_EVT_SELECT:
      switch (_cursor) {
        case 0:
          wifi_scanner_init();
          wifi_scanner_start();
          _wifiScanActive = true;
          _setState(STATE_WIFI_SCANNING);
          break;
        case 1: _setState(STATE_WIFI_CHANNEL_OCC); break;
        case 2: _setState(STATE_MAIN_MENU); break;
      }
      break;
    default: break;
  }
}

static void _handleWifiResults(ButtonEvent evt) {
  switch (evt) {
    case BTN_EVT_UP:    if (_cursor > 0) _cursor--; break;
    case BTN_EVT_DOWN:  if (_cursor < _wifiCount - 1) _cursor++; break;
    case BTN_EVT_BACK:  _setState(STATE_WIFI_MENU); return;
    case BTN_EVT_SELECT: _setState(STATE_WIFI_DETAIL); break;
    default: break;
  }
  _setCursorBounds(_wifiCount > 0 ? _wifiCount - 1 : 0);
}

static void _handleNrfMenu(ButtonEvent evt) {
  switch (evt) {
    case BTN_EVT_UP:    if (_cursor > 0) _cursor--; break;
    case BTN_EVT_DOWN:  if (_cursor < _nrfMenuCount - 1) _cursor++; break;
    case BTN_EVT_BACK:  _setState(STATE_MAIN_MENU); return;
    case BTN_EVT_SELECT:
      if (_cursor == 0) _setState(STATE_NRF_SWEEP);
      else _setState(STATE_MAIN_MENU);
      break;
    default: break;
  }
}

static void _handleAttackMenu(ButtonEvent evt) {
  switch (evt) {
    case BTN_EVT_UP:    if (_cursor > 0) _cursor--; break;
    case BTN_EVT_DOWN:  if (_cursor < _attackMenuCount - 1) _cursor++; break;
    case BTN_EVT_BACK:  _setState(STATE_MAIN_MENU); return;
    case BTN_EVT_SELECT:
      switch (_cursor) {
        case 0: _setState(STATE_WIFI_DEAUTH_SELECT); break;
        case 1: _setState(STATE_BEACON_FLOOD_CONFIRM); break;
        case 2: _setState(STATE_EVIL_TWIN_SELECT); break;
        case 3: _setState(STATE_NRF_JAM_MENU); break;
        case 4: _setState(STATE_MAIN_MENU); break;
      }
      break;
    default: break;
  }
  _setCursorBounds(_attackMenuCount - 1);
}

static void _handleSettings(ButtonEvent evt) {
  switch (evt) {
    case BTN_EVT_UP:    if (_cursor > 0) _cursor--; break;
    case BTN_EVT_DOWN:  if (_cursor < 3) _cursor++; break;
    case BTN_EVT_BACK:  _setState(STATE_MAIN_MENU); return;
    case BTN_EVT_SELECT:
      if (_cursor == 0) {
        _brightness = (_brightness >= 230) ? 25 : _brightness + 25;
        display_setBrightness(_brightness);
      }
      if (_cursor == 1) {
        _scanInterval = (_scanInterval >= 30000) ? 1000 : _scanInterval + 1000;
      }
      if (_cursor == 2) {
        display_clear();
        display_centerText("Deep Sleep...", 28, 1);
        display_update();
        delay(1000);
        ESP.deepSleep(0);
      }
      if (_cursor == 3) _setState(STATE_MAIN_MENU);
      break;
    default: break;
  }
}

// ═════════════════════════════════════════════════════════════════
//  PUBLIC API
// ═════════════════════════════════════════════════════════════════

void menu_init() {
  _setState(STATE_SPLASH);
  DBGLN(F("[MENU] ShadowNet PRO menu initialized"));
}

void menu_update(ButtonEvent evt) {
  unsigned long now = millis();

  // ── Splash auto-transition ──────────────────────────────────
  if (_state == STATE_SPLASH) {
    if (now - _stateEnterTime > SPLASH_DURATION_MS) {
      _setState(STATE_MAIN_MENU);
    }
    return;
  }

  // ── Non-blocking refresh ────────────────────────────────────
  if (now - _lastRender < UI_REFRESH_MS && evt == BTN_NONE) return;
  _lastRender = now;

  // ── WiFi scan completion ────────────────────────────────────
  if (_state == STATE_WIFI_SCANNING && _wifiScanActive) {
    int count = wifi_scanner_getResults(_wifiResults, MAX_WIFI_RESULTS);
    if (count > 0) {
      _wifiCount = count;
      wifi_scanner_sortByRSSI(_wifiResults, _wifiCount);
      _wifiScanActive = false;
      _setState(STATE_WIFI_RESULTS);
    }
  }

  // ── NRF sweep ───────────────────────────────────────────────
  if (_state == STATE_NRF_SWEEP) {
    nrf_monitor_sweep(_nrfSpectrum);
    if (evt == BTN_EVT_BACK) { _setState(STATE_NRF_MENU); return; }
  }

  // ── Attack updates ──────────────────────────────────────────
  if (_state == STATE_WIFI_DEAUTH_RUNNING) {
    wifi_attacks_update();
    if (evt == BTN_EVT_BACK) { wifi_attacks_deauth_stop(); _setState(STATE_ATTACK_MENU); return; }
  }
  if (_state == STATE_BEACON_FLOOD_RUNNING) {
    wifi_attacks_beacon_update();
    if (evt == BTN_EVT_BACK) { wifi_attacks_beacon_stop(); _setState(STATE_ATTACK_MENU); return; }
  }
  if (_state == STATE_EVIL_TWIN_RUNNING) {
    evil_twin_update();
    if (evt == BTN_EVT_BACK) { evil_twin_stop(); _setState(STATE_ATTACK_MENU); return; }
  }
  if (_state == STATE_NRF_JAM_RUNNING) {
    packet_injection_update();
    if (evt == BTN_EVT_BACK) { packet_injection_jam_stop(); _setState(STATE_ATTACK_MENU); return; }
  }

  // ── Input dispatch ──────────────────────────────────────────
  if (evt != BTN_NONE) {
    switch (_state) {
      case STATE_MAIN_MENU:    _handleMainMenu(evt); break;
      case STATE_WIFI_MENU:    _handleWifiMenu(evt); break;
      case STATE_WIFI_RESULTS: _handleWifiResults(evt); break;
      case STATE_WIFI_DETAIL:
        if (evt == BTN_EVT_BACK) _setState(STATE_WIFI_RESULTS);
        break;
      case STATE_WIFI_CHANNEL_OCC:
        if (evt == BTN_EVT_BACK) _setState(STATE_WIFI_MENU);
        break;
      case STATE_NRF_MENU:     _handleNrfMenu(evt); break;
      case STATE_ATTACK_MENU:  _handleAttackMenu(evt); break;

      // Deauth select
      case STATE_WIFI_DEAUTH_SELECT:
        if (evt == BTN_EVT_BACK) { _setState(STATE_ATTACK_MENU); break; }
        if (evt == BTN_EVT_UP && _cursor > 0) _cursor--;
        if (evt == BTN_EVT_DOWN && _cursor < _wifiCount - 1) _cursor++;
        _setCursorBounds(_wifiCount > 0 ? _wifiCount - 1 : 0);
        if (evt == BTN_EVT_SELECT && _wifiCount > 0) {
          _savedDeauthCursor = _cursor;
          _setState(STATE_WIFI_DEAUTH_CONFIRM);
        }
        break;

      // Deauth confirm
      case STATE_WIFI_DEAUTH_CONFIRM:
        if (evt == BTN_EVT_BACK) _setState(STATE_WIFI_DEAUTH_SELECT);
        if (evt == BTN_EVT_SELECT) {
          wifi_attacks_init();
          wifi_attacks_deauth_start(_wifiResults[_savedDeauthCursor].bssid, _wifiResults[_savedDeauthCursor].channel);
          _setState(STATE_WIFI_DEAUTH_RUNNING);
        }
        break;

      // Beacon flood
      case STATE_BEACON_FLOOD_CONFIRM:
        if (evt == BTN_EVT_BACK) _setState(STATE_ATTACK_MENU);
        if (evt == BTN_EVT_SELECT) {
          wifi_attacks_beacon_start();
          _setState(STATE_BEACON_FLOOD_RUNNING);
        }
        break;

      // Evil twin select
      case STATE_EVIL_TWIN_SELECT:
        if (evt == BTN_EVT_BACK) { _setState(STATE_ATTACK_MENU); break; }
        if (evt == BTN_EVT_UP && _cursor > 0) _cursor--;
        if (evt == BTN_EVT_DOWN && _cursor < _wifiCount - 1) _cursor++;
        _setCursorBounds(_wifiCount > 0 ? _wifiCount - 1 : 0);
        if (evt == BTN_EVT_SELECT && _wifiCount > 0) {
          _savedETCursor = _cursor;
          _setState(STATE_EVIL_TWIN_CONFIRM);
        }
        break;

      // Evil twin confirm
      case STATE_EVIL_TWIN_CONFIRM:
        if (evt == BTN_EVT_BACK) _setState(STATE_EVIL_TWIN_SELECT);
        if (evt == BTN_EVT_SELECT) {
          evil_twin_start(
            _wifiResults[_savedETCursor].ssid,
            _wifiResults[_savedETCursor].channel,
            _wifiResults[_savedETCursor].bssid
          );
          _setState(STATE_EVIL_TWIN_RUNNING);
        }
        break;

      // NRF jam type
      case STATE_NRF_JAM_MENU:
        if (evt == BTN_EVT_BACK) { _setState(STATE_ATTACK_MENU); break; }
        if (evt == BTN_EVT_UP && _cursor > 0) _cursor--;
        if (evt == BTN_EVT_DOWN && _cursor < _jamModeCount - 1) _cursor++;
        _setCursorBounds(_jamModeCount - 1);
        if (evt == BTN_EVT_SELECT) {
          if (_cursor == 3) { _setState(STATE_ATTACK_MENU); break; }
          _jamMode = (NrfJamMode)_cursor;
          _setState(STATE_NRF_JAM_CONFIRM);
        }
        break;

      // NRF jam confirm
      case STATE_NRF_JAM_CONFIRM:
        if (evt == BTN_EVT_BACK) _setState(STATE_NRF_JAM_MENU);
        if (evt == BTN_EVT_SELECT) {
          packet_injection_init();
          packet_injection_jam_start(_jamStartCh, _jamEndCh, _jamMode);
          _setState(STATE_NRF_JAM_RUNNING);
        }
        break;

      // Packet counter
      case STATE_PACKET_COUNTER:
        if (evt == BTN_EVT_BACK) { wifi_attacks_stopPacketCounter(); _setState(STATE_MAIN_MENU); }
        if (evt == BTN_EVT_SELECT) wifi_attacks_resetPacketCount();
        break;

      // System info
      case STATE_SYSINFO:
        if (evt == BTN_EVT_BACK) _setState(STATE_MAIN_MENU);
        break;

      // About
      case STATE_ABOUT:
        if (evt == BTN_EVT_BACK) _setState(STATE_MAIN_MENU);
        break;

      // Settings
      case STATE_SETTINGS:
        _handleSettings(evt);
        break;

      default: break;
    }
  }

  // ── Render ──────────────────────────────────────────────────
  display_clear();

  switch (_state) {
    case STATE_MAIN_MENU:           _renderMainMenu();       break;
    case STATE_WIFI_MENU:           _renderWifiMenu();       break;
    case STATE_WIFI_SCANNING:       _renderWifiScanning();   break;
    case STATE_WIFI_RESULTS:        _renderWifiResults();    break;
    case STATE_WIFI_DETAIL:         _renderWifiDetail();     break;
    case STATE_WIFI_CHANNEL_OCC:    _renderWifiChannelOcc(); break;
    case STATE_NRF_MENU:            _renderNrfMenu();        break;
    case STATE_NRF_SWEEP:           _renderNrfSweep();       break;
    case STATE_ATTACK_MENU:         _renderAttackMenu();     break;
    case STATE_WIFI_DEAUTH_SELECT:  _renderDeauthSelect();   break;
    case STATE_WIFI_DEAUTH_CONFIRM: _renderDeauthConfirm();  break;
    case STATE_WIFI_DEAUTH_RUNNING: _renderDeauthRunning();  break;
    case STATE_BEACON_FLOOD_CONFIRM:_renderBeaconConfirm();  break;
    case STATE_BEACON_FLOOD_RUNNING:_renderBeaconRunning();  break;
    case STATE_EVIL_TWIN_SELECT:    _renderETSelect();       break;
    case STATE_EVIL_TWIN_CONFIRM:   _renderETConfirm();      break;
    case STATE_EVIL_TWIN_RUNNING:   _renderETRunning();      break;
    case STATE_NRF_JAM_MENU:        _renderNrfJamMenu();     break;
    case STATE_NRF_JAM_CONFIRM:     _renderNrfJamConfirm();  break;
    case STATE_NRF_JAM_RUNNING:     _renderNrfJamRunning();  break;
    case STATE_PACKET_COUNTER:      _renderPacketCounter();  break;
    case STATE_SYSINFO:             _renderSysInfo();        break;
    case STATE_ABOUT:               _renderAbout();          break;
    case STATE_SETTINGS:            _renderSettings();       break;
    default: break;
  }

  display_update();
}

AppState menu_getState() {
  return _state;
}

const char* menu_getModeLabel() {
  switch (_state) {
    case STATE_WIFI_SCANNING:
    case STATE_WIFI_RESULTS:
    case STATE_WIFI_DETAIL:
    case STATE_WIFI_CHANNEL_OCC:      return "WiFi";
    case STATE_NRF_SWEEP:             return "NRF";
    case STATE_WIFI_DEAUTH_RUNNING:   return "DEAUTH";
    case STATE_BEACON_FLOOD_RUNNING:  return "BEACON";
    case STATE_EVIL_TWIN_RUNNING:     return "PHISH";
    case STATE_NRF_JAM_RUNNING:       return "JAM";
    case STATE_PACKET_COUNTER:        return "PKTS";
    case STATE_SYSINFO:               return "INFO";
    case STATE_ABOUT:                 return "ABOUT";
    case STATE_SETTINGS:              return "SET";
    default:                          return "MENU";
  }
}
