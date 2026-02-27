/*
 * menu_manager.cpp — State-machine based UI / menu system
 *
 * ================================================================
 *  This firmware is intended for educational cybersecurity research,
 *  wireless analysis, and defensive learning in controlled
 *  environments only.
 * ================================================================
 *
 * Implements the entire user interface as a finite state machine.
 * Each state has its own render function and input handler.  The
 * display is refreshed non-blockingly using millis().
 */

#include "menu_manager.h"
#include "display_manager.h"
#include "wifi_scanner.h"
#include "nrf_monitor.h"
#include "wifi_attacks.h"
#include "packet_injection.h"
#include "battery_manager.h"

// ── Current state & cursor tracking ──────────────────────────────
static AppState _state = STATE_SPLASH;
static uint8_t  _cursor = 0;          // Current menu selection index
static uint8_t  _scroll = 0;          // Scroll offset for long lists
static unsigned long _stateEnterTime = 0;
static unsigned long _lastRender = 0;

// ── WiFi scan results cache ──────────────────────────────────────
static WifiNetwork _wifiResults[MAX_WIFI_RESULTS];
static uint8_t     _wifiCount = 0;
static bool        _wifiScanActive = false;


// ── NRF spectrum data ────────────────────────────────────────────
static uint8_t _nrfSpectrum[NRF_NUM_CHANNELS];

// ── Settings ─────────────────────────────────────────────────────
static uint8_t _brightness = DEFAULT_BRIGHTNESS;
static uint16_t _scanInterval = WIFI_SCAN_INTERVAL_MS;

// ── NRF jam parameters ──────────────────────────────────────────
static uint8_t _jamStartCh = 0;
static uint8_t _jamEndCh   = 125;
static NrfJamMode _jamMode = NJAM_MODE_HOPPING;

// ── Helpers ──────────────────────────────────────────────────────
static void _setState(AppState s) {
  _state = s;
  _cursor = 0;
  _scroll = 0;
  _stateEnterTime = millis();
}

static void _setCursorBounds(uint8_t maxIdx) {
  if (_cursor > maxIdx) _cursor = maxIdx;
  // Adjust scroll to keep cursor visible
  if (_cursor < _scroll) _scroll = _cursor;
  if (_cursor >= _scroll + MENU_VISIBLE_ITEMS) _scroll = _cursor - MENU_VISIBLE_ITEMS + 1;
}

// ═════════════════════════════════════════════════════════════════
//  RENDER FUNCTIONS — one per state
// ═════════════════════════════════════════════════════════════════

// ── Main Menu ────────────────────────────────────────────────────
static const char* _mainItems[] = {
  "WiFi Scanner",
  "2.4GHz Monitor",
  "Attacks",
  "Packet Counter",
  "System Info",
  "Settings"
};
static const uint8_t _mainCount = 6;

static void _renderMainMenu() {
  display_statusBar(battery_getPercent(), "MENU");
  display_list(_mainItems, _mainCount, _cursor, _scroll);
}

// ── WiFi Sub-Menu ────────────────────────────────────────────────
static const char* _wifiMenuItems[] = {
  "Scan Networks",
  "Channel Occupancy",
  "Back"
};
static const uint8_t _wifiMenuCount = 3;

static void _renderWifiMenu() {
  display_statusBar(battery_getPercent(), "WiFi");
  display_list(_wifiMenuItems, _wifiMenuCount, _cursor, _scroll);
}

// ── WiFi Scanning ────────────────────────────────────────────────
static void _renderWifiScanning() {
  display_progress("WiFi Scanning...", 0);
}

// ── WiFi Results List ─────────────────────────────────────────────
static char _wifiLabels[MAX_WIFI_RESULTS][22];
static const char* _wifiLabelPtrs[MAX_WIFI_RESULTS];

static void _renderWifiResults() {
  display_statusBar(battery_getPercent(), "WiFi");

  if (_wifiCount == 0) {
    display_centerText("No networks found", 30, 1);
    return;
  }

  // Build display labels:  "SSID  -XXdBm"
  for (uint8_t i = 0; i < _wifiCount; i++) {
    snprintf(_wifiLabels[i], 22, "%-12.12s %ddBm",
             _wifiResults[i].ssid, _wifiResults[i].rssi);
    _wifiLabelPtrs[i] = _wifiLabels[i];
  }
  display_list(_wifiLabelPtrs, _wifiCount, _cursor, _scroll);
}

// ── WiFi Network Detail ──────────────────────────────────────────
static void _renderWifiDetail() {
  if (_cursor >= _wifiCount) return;
  WifiNetwork& n = _wifiResults[_cursor];

  display_statusBar(battery_getPercent(), "Detail");
  OLED_CLASS& oled = display_get();
  int16_t y = HEADER_HEIGHT + 2;

  oled.setCursor(0, y); oled.print(F("SSID: ")); oled.print(n.ssid);
  y += 10;
  oled.setCursor(0, y);
  oled.printf("BSSID:%02X:%02X:%02X:%02X:%02X:%02X", n.bssid[0], n.bssid[1],
              n.bssid[2], n.bssid[3], n.bssid[4], n.bssid[5]);
  y += 10;
  oled.setCursor(0, y); oled.print(F("RSSI: ")); oled.print(n.rssi); oled.print(F(" dBm"));
  y += 10;
  oled.setCursor(0, y); oled.print(F("Ch: ")); oled.print(n.channel);
  oled.print(F("  Enc: ")); oled.print(wifi_scanner_encLabel(n.encType));
  y += 10;

  // Signal strength bar
  uint8_t pct = map(constrain(n.rssi, -100, -30), -100, -30, 0, 100);
  display_barGraph(0, y, 128, 8, pct);
}

// ── WiFi Channel Occupancy ───────────────────────────────────────
static void _renderWifiChannelOcc() {
  display_statusBar(battery_getPercent(), "ChOcc");
  OLED_CLASS& oled = display_get();

  uint8_t chCount[15];
  wifi_scanner_channelOccupancy(_wifiResults, _wifiCount, chCount);

  // Find max for scaling
  uint8_t maxC = 1;
  for (uint8_t i = 1; i <= 14; i++) {
    if (chCount[i] > maxC) maxC = chCount[i];
  }

  int16_t barW = 8;
  int16_t gap = 1;
  int16_t startX = 2;
  int16_t bottom = 58;
  int16_t maxH = 38;

  for (uint8_t i = 1; i <= 14; i++) {
    int16_t x = startX + (i - 1) * (barW + gap);
    int16_t h = map(chCount[i], 0, maxC, 0, maxH);
    if (h > 0) {
      oled.fillRect(x, bottom - h, barW, h, OLED_COLOR_WHITE);
    }
    // Channel number
    oled.setTextSize(1);
    oled.setCursor(x + 1, 60);
    if (i < 10) oled.print(i);
    else { oled.setTextSize(1); oled.setCursor(x - 1, 60); oled.print(i); }
  }
}

// ── BLE Scanning ─────────────────────────────────────────────────
// ── BLE Results ──────────────────────────────────────────────────

// ── NRF Sub-Menu ─────────────────────────────────────────────────
static const char* _nrfMenuItems[] = {
  "Channel Sweep",
  "Back"
};
static const uint8_t _nrfMenuCount = 2;

static void _renderNrfMenu() {
  display_statusBar(battery_getPercent(), "NRF24");
  display_list(_nrfMenuItems, _nrfMenuCount, _cursor, _scroll);
}

// ── NRF Spectrum Sweep ───────────────────────────────────────────
static void _renderNrfSweep() {
  display_statusBar(battery_getPercent(), "2.4GHz");
  OLED_CLASS& oled = display_get();

  // Draw spectrum bars
  int16_t bottom = 58;
  int16_t maxH = 40;
  uint8_t peak = nrf_monitor_peakChannel(_nrfSpectrum);

  for (uint8_t i = 0; i < NRF_NUM_CHANNELS; i++) {
    int16_t x = map(i, 0, NRF_NUM_CHANNELS, 0, SCREEN_WIDTH);
    display_vertBar(x, bottom, maxH, _nrfSpectrum[i], 10);
  }

  // Peak label
  char peakBuf[24];
  snprintf(peakBuf, 24, "Peak: ch%d (%dMHz)", peak, 2400 + peak);
  oled.setCursor(0, 60);
  oled.print(peakBuf);
}

// ── Attack Menu ──────────────────────────────────────────────────
static const char* _attackMenuItems[] = {
  "WiFi Deauth",
  "2.4GHz Jam",
  "Back"
};
static const uint8_t _attackMenuCount = 4;

static void _renderAttackMenu() {
  display_statusBar(battery_getPercent(), "ATTACK");
  display_list(_attackMenuItems, _attackMenuCount, _cursor, _scroll);
}

// ── WiFi Deauth Target Selection ─────────────────────────────────
static void _renderDeauthSelect() {
  display_statusBar(battery_getPercent(), "DEAUTH");
  if (_wifiCount == 0) {
    display_centerText("No targets!", 26, 1);
    display_centerText("Scan WiFi first", 38, 1);
    return;
  }
  for (uint8_t i = 0; i < _wifiCount; i++) {
    snprintf(_wifiLabels[i], 22, "%-12.12s ch%d",
             _wifiResults[i].ssid, _wifiResults[i].channel);
    _wifiLabelPtrs[i] = _wifiLabels[i];
  }
  display_list(_wifiLabelPtrs, _wifiCount, _cursor, _scroll);
}

// ── Deauth Confirm Dialog ────────────────────────────────────────
static void _renderDeauthConfirm() {
  display_confirmDialog(
    "! DEAUTH !",
    "Educational Only!",
    "Is this YOUR device?"
  );
}

// ── Deauth Running ───────────────────────────────────────────────
static void _renderDeauthRunning() {
  display_statusBar(battery_getPercent(), "DEAUTH");
  OLED_CLASS& oled = display_get();

  oled.setCursor(0, HEADER_HEIGHT + 4);
  oled.print(F("Target: "));
  if (_cursor < _wifiCount) {
    oled.print(_wifiResults[_cursor].ssid);
  }

  oled.setCursor(0, HEADER_HEIGHT + 16);
  oled.print(F("Frames: "));
  oled.print(wifi_attacks_getFrameCount());

  oled.setCursor(0, HEADER_HEIGHT + 28);
  oled.print(F("Status: ACTIVE"));

  oled.setCursor(0, HEADER_HEIGHT + 42);
  oled.print(F("[BACK] to stop"));
}

// ── BLE Flood Confirm ────────────────────────────────────────────
// ── BLE Flood Running ────────────────────────────────────────────
// ── NRF Jam Menu ─────────────────────────────────────────────────
static const char* _jamModeItems[] = {
  "Constant (1ch)",
  "Hopping (rand)",
  "Sweep (all)",
  "Back"
};
static const uint8_t _jamModeCount = 4;

static void _renderNrfJamMenu() {
  display_statusBar(battery_getPercent(), "NRF JAM");
  display_list(_jamModeItems, _jamModeCount, _cursor, _scroll);
}

// ── NRF Jam Confirm ──────────────────────────────────────────────
static void _renderNrfJamConfirm() {
  display_confirmDialog(
    "! 2.4GHz JAM !",
    "Educational Only!",
    "Is this YOUR network?"
  );
}

// ── NRF Jam Running ──────────────────────────────────────────────
static void _renderNrfJamRunning() {
  display_statusBar(battery_getPercent(), "JAMMING");
  OLED_CLASS& oled = display_get();

  const char* modeStr = "???";
  switch (_jamMode) {
    case NJAM_MODE_CONSTANT: modeStr = "Constant"; break;
    case NJAM_MODE_HOPPING:  modeStr = "Hopping";  break;
    case NJAM_MODE_SWEEP:    modeStr = "Sweep";    break;
  }

  oled.setCursor(0, HEADER_HEIGHT + 4);
  oled.print(F("Mode: ")); oled.print(modeStr);

  oled.setCursor(0, HEADER_HEIGHT + 16);
  oled.print(F("Ch: ")); oled.print(_jamStartCh);
  oled.print(F("-")); oled.print(_jamEndCh);

  oled.setCursor(0, HEADER_HEIGHT + 28);
  oled.print(F("TX: ")); oled.print(packet_injection_getTxCount());

  oled.setCursor(0, HEADER_HEIGHT + 42);
  oled.print(F("[BACK] to stop"));
}

// ── Packet Counter ───────────────────────────────────────────────
static void _renderPacketCounter() {
  display_statusBar(battery_getPercent(), "PKTS");
  OLED_CLASS& oled = display_get();

  oled.setCursor(0, HEADER_HEIGHT + 4);
  oled.print(F("Packet Counter"));

  oled.setCursor(0, HEADER_HEIGHT + 18);
  oled.print(F("Total: "));
  oled.print(wifi_attacks_getPacketCount());

  bool interference = wifi_attacks_detectInterference();
  oled.setCursor(0, HEADER_HEIGHT + 32);
  oled.print(F("Deauth Alert: "));
  oled.print(interference ? "YES!" : "No");

  oled.setCursor(0, HEADER_HEIGHT + 46);
  oled.print(F("[SEL]Reset [BACK]Exit"));
}

// ── System Info ──────────────────────────────────────────────────
static void _renderSysInfo() {
  display_statusBar(battery_getPercent(), "INFO");
  OLED_CLASS& oled = display_get();
  int16_t y = HEADER_HEIGHT + 2;

  oled.setCursor(0, y);
  oled.printf("Battery: %.2fV (%d%%)", battery_getVoltage(), battery_getPercent());
  y += 10;

  oled.setCursor(0, y);
  oled.printf("Heap: %lu B", (unsigned long)ESP.getFreeHeap());
  y += 10;

  oled.setCursor(0, y);
  oled.print(F("FW: ")); oled.print(FW_NAME); oled.print(F(" v")); oled.print(FW_VERSION);
  y += 10;

  unsigned long up = millis() / 1000;
  oled.setCursor(0, y);
  oled.printf("Up: %luh %lum %lus", up / 3600, (up % 3600) / 60, up % 60);
  y += 10;

  oled.setCursor(0, y);
  oled.print(F("NRF: ")); oled.print(nrf_monitor_isConnected() ? "OK" : "N/A");
}

// ── Settings ─────────────────────────────────────────────────────
static void _renderSettings() {
  display_statusBar(battery_getPercent(), "SET");
  OLED_CLASS& oled = display_get();
  int16_t y = HEADER_HEIGHT + 2;

  // Brightness
  bool sel = (_cursor == 0);
  if (sel) { oled.fillRect(0, y - 1, 128, ITEM_HEIGHT, OLED_COLOR_WHITE); oled.setTextColor(OLED_COLOR_BLACK); }
  oled.setCursor(4, y);
  oled.printf("Brightness: %d", _brightness);
  oled.setTextColor(OLED_COLOR_WHITE);
  y += ITEM_HEIGHT;

  // Scan interval
  sel = (_cursor == 1);
  if (sel) { oled.fillRect(0, y - 1, 128, ITEM_HEIGHT, OLED_COLOR_WHITE); oled.setTextColor(OLED_COLOR_BLACK); }
  oled.setCursor(4, y);
  oled.printf("Scan Intv: %ds", _scanInterval / 1000);
  oled.setTextColor(OLED_COLOR_WHITE);
  y += ITEM_HEIGHT;

  // Deep sleep
  sel = (_cursor == 2);
  if (sel) { oled.fillRect(0, y - 1, 128, ITEM_HEIGHT, OLED_COLOR_WHITE); oled.setTextColor(OLED_COLOR_BLACK); }
  oled.setCursor(4, y);
  oled.print(F("Deep Sleep Now"));
  oled.setTextColor(OLED_COLOR_WHITE);
  y += ITEM_HEIGHT;

  // Back
  sel = (_cursor == 3);
  if (sel) { oled.fillRect(0, y - 1, 128, ITEM_HEIGHT, OLED_COLOR_WHITE); oled.setTextColor(OLED_COLOR_BLACK); }
  oled.setCursor(4, y);
  oled.print(F("Back"));
  oled.setTextColor(OLED_COLOR_WHITE);
}

// ═════════════════════════════════════════════════════════════════
//  STATE MACHINE — input handling + state transitions
// ═════════════════════════════════════════════════════════════════

static uint8_t _savedDeauthCursor = 0;  // Remember selected target for deauth

// Forward declarations
static void _handleMainMenu(ButtonEvent evt);
static void _handleWifiMenu(ButtonEvent evt);
static void _handleWifiResults(ButtonEvent evt);
static void _handleNrfMenu(ButtonEvent evt);
static void _handleAttackMenu(ButtonEvent evt);
static void _handleSettings(ButtonEvent evt);

static void _handleMainMenu(ButtonEvent evt) {
  switch (evt) {
    case BTN_EVT_UP:    if (_cursor > 0) _cursor--; break;
    case BTN_EVT_DOWN:  if (_cursor < _mainCount - 1) _cursor++; break;
    case BTN_EVT_SELECT:
      switch (_cursor) {
        case 0: _setState(STATE_WIFI_MENU); break;
        case 1: _setState(STATE_NRF_MENU); break;
        case 2: _setState(STATE_ATTACK_MENU); break;
        case 3:
          wifi_attacks_startPacketCounter();
          _setState(STATE_PACKET_COUNTER);
          break;
        case 4: _setState(STATE_SYSINFO); break;
        case 5: _setState(STATE_SETTINGS); break;
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
        case 0:  // Scan Networks
          wifi_scanner_init();
          wifi_scanner_start();
          _wifiScanActive = true;
          _setState(STATE_WIFI_SCANNING);
          break;
        case 1:  // Channel Occupancy (uses cached results)
          _setState(STATE_WIFI_CHANNEL_OCC);
          break;
        case 2:  // Back
          _setState(STATE_MAIN_MENU);
          break;
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
    case BTN_EVT_SELECT:
      _setState(STATE_WIFI_DETAIL);
      break;
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
        case 0:  // WiFi Deauth — need targets
          _setState(STATE_WIFI_DEAUTH_SELECT);
          break;
        case 1:  // 2.4GHz Jam
          _setState(STATE_NRF_JAM_MENU);
          break;
        case 2:  // Back
          _setState(STATE_MAIN_MENU);
          break;
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
      if (_cursor == 0 && _brightness <= 230) { _brightness += 25; display_setBrightness(_brightness); }
      else if (_cursor == 0) { _brightness = 25; display_setBrightness(_brightness); }
      
      if (_cursor == 1 && _scanInterval < 30000) _scanInterval += 1000;
      else if (_cursor == 1) _scanInterval = 1000;

      if (_cursor == 2) {
        // Deep sleep
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
  DBGLN(F("[MENU] Menu system initialized"));
}

/*
 * menu_update()
 * Central update function — dispatches render and input to the
 * current state.  Non-blocking.
 */
void menu_update(ButtonEvent evt) {
  unsigned long now = millis();

  // ── Splash screen auto-transition ───────────────────────────
  if (_state == STATE_SPLASH) {
    if (now - _stateEnterTime > SPLASH_DURATION_MS) {
      _setState(STATE_MAIN_MENU);
    }
    return;  // Don't render anything else during splash
  }

  // ── Non-blocking UI refresh ─────────────────────────────────
  if (now - _lastRender < UI_REFRESH_MS && evt == BTN_NONE) return;
  _lastRender = now;

  // ── WiFi scan completion check ──────────────────────────────
  if (_state == STATE_WIFI_SCANNING && _wifiScanActive) {
    int count = wifi_scanner_getResults(_wifiResults, MAX_WIFI_RESULTS);
    if (count > 0) {
      _wifiCount = count;
      wifi_scanner_sortByRSSI(_wifiResults, _wifiCount);
      _wifiScanActive = false;
      _setState(STATE_WIFI_RESULTS);
    }
  }

    // ── NRF sweep continuous ────────────────────────────────────
  if (_state == STATE_NRF_SWEEP) {
    nrf_monitor_sweep(_nrfSpectrum);
    if (evt == BTN_EVT_BACK) {
      _setState(STATE_NRF_MENU);
      return;
    }
  }

  // ── Attack updates (non-blocking) ──────────────────────────
  if (_state == STATE_WIFI_DEAUTH_RUNNING) {
    wifi_attacks_update();
    if (evt == BTN_EVT_BACK) {
      wifi_attacks_deauth_stop();
      _setState(STATE_ATTACK_MENU);
      return;
    }
  }

  if (_state == STATE_NRF_JAM_RUNNING) {
    packet_injection_update();
    if (evt == BTN_EVT_BACK) {
      packet_injection_jam_stop();
      _setState(STATE_ATTACK_MENU);
      return;
    }
  }

  // ── Input handling ──────────────────────────────────────────
  if (evt != BTN_NONE) {
    switch (_state) {
      case STATE_MAIN_MENU:          _handleMainMenu(evt); break;
      case STATE_WIFI_MENU:          _handleWifiMenu(evt); break;
      case STATE_WIFI_RESULTS:       _handleWifiResults(evt); break;
      case STATE_WIFI_DETAIL:
        if (evt == BTN_EVT_BACK) _setState(STATE_WIFI_RESULTS);
        break;
      case STATE_WIFI_CHANNEL_OCC:
        if (evt == BTN_EVT_BACK) _setState(STATE_WIFI_MENU);
        break;
      
      
      case STATE_NRF_MENU:           _handleNrfMenu(evt); break;
      case STATE_ATTACK_MENU:        _handleAttackMenu(evt); break;

      // Deauth select target
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
        if (evt == BTN_EVT_BACK || evt == BTN_EVT_SELECT) {
          _setState(STATE_WIFI_DEAUTH_SELECT);
        }
        if (evt == BTN_EVT_SELECT) {
          wifi_attacks_init();
          wifi_attacks_deauth_start(
            _wifiResults[_savedDeauthCursor].bssid,
            _wifiResults[_savedDeauthCursor].channel
          );
          _cursor = _savedDeauthCursor;
          _setState(STATE_WIFI_DEAUTH_RUNNING);
        }
        break;

      // NRF jam type selection
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
        if (evt == BTN_EVT_BACK || evt == BTN_EVT_SELECT) {
          _setState(STATE_NRF_JAM_MENU);
        }
        if (evt == BTN_EVT_SELECT) {
          packet_injection_init();
          packet_injection_jam_start(_jamStartCh, _jamEndCh, _jamMode);
          _setState(STATE_NRF_JAM_RUNNING);
        }
        break;

      // Packet counter
      case STATE_PACKET_COUNTER:
        if (evt == BTN_EVT_BACK) {
          wifi_attacks_stopPacketCounter();
          _setState(STATE_MAIN_MENU);
        }
        if (evt == BTN_EVT_SELECT) {
          wifi_attacks_resetPacketCount();
        }
        break;

      // System info
      case STATE_SYSINFO:
        if (evt == BTN_EVT_BACK) _setState(STATE_MAIN_MENU);
        break;

      // Settings
      case STATE_SETTINGS:
        _handleSettings(evt);
        break;

      default: break;
    }
  }

  // ── Render current state ────────────────────────────────────
  display_clear();

  switch (_state) {
    case STATE_MAIN_MENU:          _renderMainMenu();        break;
    case STATE_WIFI_MENU:          _renderWifiMenu();         break;
    case STATE_WIFI_SCANNING:      _renderWifiScanning();     break;
    case STATE_WIFI_RESULTS:       _renderWifiResults();      break;
    case STATE_WIFI_DETAIL:        _renderWifiDetail();       break;
    case STATE_WIFI_CHANNEL_OCC:   _renderWifiChannelOcc();   break;
    
    
    
    case STATE_NRF_MENU:           _renderNrfMenu();          break;
    case STATE_NRF_SWEEP:          _renderNrfSweep();         break;
    case STATE_ATTACK_MENU:        _renderAttackMenu();       break;
    case STATE_WIFI_DEAUTH_SELECT: _renderDeauthSelect();     break;
    case STATE_WIFI_DEAUTH_CONFIRM:_renderDeauthConfirm();    break;
    case STATE_WIFI_DEAUTH_RUNNING:_renderDeauthRunning();    break;
    case STATE_NRF_JAM_MENU:       _renderNrfJamMenu();       break;
    case STATE_NRF_JAM_CONFIRM:    _renderNrfJamConfirm();    break;
    case STATE_NRF_JAM_RUNNING:    _renderNrfJamRunning();    break;
    case STATE_PACKET_COUNTER:     _renderPacketCounter();    break;
    case STATE_SYSINFO:            _renderSysInfo();          break;
    case STATE_SETTINGS:           _renderSettings();         break;
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
    case STATE_WIFI_CHANNEL_OCC:     return "WiFi";
    case STATE_NRF_SWEEP:            return "NRF";
    case STATE_WIFI_DEAUTH_RUNNING:  return "DEAUTH";
    case STATE_NRF_JAM_RUNNING:      return "JAM";
    case STATE_PACKET_COUNTER:       return "PKTS";
    case STATE_SYSINFO:              return "INFO";
    case STATE_SETTINGS:             return "SET";
    default:                         return "MENU";
  }
}
