/*
 * display_manager.cpp — OLED display driver implementation
 *
 * Wraps OLED_CLASS for the 128×64 OLED, providing higher-level
 * drawing helpers used throughout the menu system.
 */

#include "display_manager.h"

// Global display object
static OLED_CLASS _oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
static bool _oledOk = false;

/*
 * Battery icon bitmap (12×6 pixels, PROGMEM)
 */
static const uint8_t PROGMEM _battIcon[] = {
  0b11111111, 0b11110000,
  0b10000000, 0b00010000,
  0b10000000, 0b00111000,
  0b10000000, 0b00111000,
  0b10000000, 0b00010000,
  0b11111111, 0b11110000,
};

// ────────────────────────────────────────────────────────────────

bool display_init() {
  Wire.begin(SDA_PIN, SCL_PIN);
#ifdef USE_SH1106
  if (!_oled.begin(OLED_ADDR, true)) { // true = reset
#else
  if (!_oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
#endif
    DBGLN(F("[OLED] display init FAILED"));
    _oledOk = false;
    return false;
  }
  _oledOk = true;
  _oled.setTextColor(OLED_COLOR_WHITE);
  _oled.setTextSize(1);
  _oled.cp437(true);
  _oled.clearDisplay();
  _oled.display();
  DBGLN(F("[OLED] Display initialized"));
  return true;
}

bool display_isOk() {
  return _oledOk;
}

OLED_CLASS& display_get() {
  return _oled;
}

void display_clear() {
  if (!_oledOk) return;
  _oled.clearDisplay();
}

void display_update() {
  if (!_oledOk) return;
  _oled.display();
}

void display_setBrightness(uint8_t brightness) {
  if (!_oledOk) return;
#ifdef USE_SH1106
  _oled.setContrast(brightness);
#else
  _oled.ssd1306_command(SSD1306_SETCONTRAST);
  _oled.ssd1306_command(brightness);
#endif
}

void display_splash() {
  if (!_oledOk) return;

  // Phase 1: Matrix-style init
  _oled.clearDisplay();
  _oled.setTextSize(1);
  _oled.setTextColor(OLED_COLOR_WHITE);
  
  _oled.setCursor(0, 0);
  _oled.print(F("[SYS] Booting kernel..."));
  _oled.display(); delay(300);
  
  _oled.setCursor(0, 9);
  _oled.print(F("[NET] Initializing RF..."));
  _oled.display(); delay(250);
  
  _oled.setCursor(0, 18);
  _oled.print(F("[SEC] Loading modules..."));
  _oled.display(); delay(250);
  
  _oled.setCursor(0, 27);
  _oled.print(F("[AUTH] Authenticating..."));
  _oled.display(); delay(400);

  // Loading bar
  _oled.drawRect(10, 40, 108, 8, OLED_COLOR_WHITE);
  for (int i = 0; i <= 100; i += 5) {
    int16_t w = map(i, 0, 100, 0, 106);
    _oled.fillRect(11, 41, w, 6, OLED_COLOR_WHITE);
    _oled.display();
    delay(30);
  }
  
  _oled.setCursor(20, 52);
  _oled.print(F("ACCESS GRANTED"));
  _oled.display(); delay(600);

  // Phase 2: Branding
  _oled.clearDisplay();
  
  // ShadowNet PRO title
  _oled.setTextSize(1);
  _oled.setCursor(14, 2);
  _oled.print(F(">> ShadowNet PRO <<"));
  
  // Version
  _oled.setCursor(40, 14);
  _oled.print(F("v")); _oled.print(F(FW_VERSION));
  
  // Divider
  for (int x = 10; x < 118; x += 2) {
    _oled.drawPixel(x, 23, OLED_COLOR_WHITE);
  }
  
  // Creator info
  _oled.setCursor(6, 27);
  _oled.print(F("By: Shawal A. Mohmand"));
  
  _oled.setCursor(10, 38);
  _oled.print(F("+92 304 975 8182"));
  
  // Divider
  for (int x = 10; x < 118; x += 2) {
    _oled.drawPixel(x, 48, OLED_COLOR_WHITE);
  }
  
  _oled.setCursor(4, 52);
  _oled.print(F("Educational Use Only"));
  
  _oled.display();
}


void display_statusBar(uint8_t battPct, const char* modeLabel) {
  if (!_oledOk) return;
  _oled.fillRect(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, OLED_COLOR_WHITE);
  _oled.drawBitmap(2, 3, _battIcon, 12, 6, OLED_COLOR_BLACK);

  uint8_t fillW = map(constrain(battPct, 0, 100), 0, 100, 0, 8);
  _oled.fillRect(4, 4, fillW, 4, OLED_COLOR_BLACK);

  char buf[6];
  snprintf(buf, sizeof(buf), "%d%%", battPct);
  _oled.setTextSize(1);
  _oled.setTextColor(OLED_COLOR_BLACK);
  _oled.setCursor(16, 2);
  _oled.print(buf);

  int16_t x1, y1;
  uint16_t tw, th;
  _oled.getTextBounds(modeLabel, 0, 0, &x1, &y1, &tw, &th);
  _oled.setCursor(SCREEN_WIDTH - tw - 4, 2);
  _oled.print(modeLabel);

  _oled.setTextColor(OLED_COLOR_WHITE);
}

void display_barGraph(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t pct) {
  if (!_oledOk) return;
  _oled.drawRect(x, y, w, h, OLED_COLOR_WHITE);
  int16_t fillW = map(constrain(pct, 0, 100), 0, 100, 0, w - 2);
  _oled.fillRect(x + 1, y + 1, fillW, h - 2, OLED_COLOR_WHITE);
}

void display_vertBar(int16_t x, int16_t bottom, int16_t maxH, uint8_t value, uint8_t maxVal) {
  if (!_oledOk) return;
  if (maxVal == 0) return;
  int16_t h = map(constrain(value, 0, maxVal), 0, maxVal, 0, maxH);
  if (h > 0) {
    _oled.fillRect(x, bottom - h, 1, h, OLED_COLOR_WHITE);
  }
}

void display_list(const char* items[], uint8_t count, uint8_t selected, uint8_t scrollOffset) {
  if (!_oledOk) return;
  int16_t y = HEADER_HEIGHT + 2;
  for (uint8_t i = 0; i < MENU_VISIBLE_ITEMS && (scrollOffset + i) < count; i++) {
    uint8_t idx = scrollOffset + i;
    if (idx == selected) {
      _oled.fillRect(0, y - 1, SCREEN_WIDTH, ITEM_HEIGHT, OLED_COLOR_WHITE);
      _oled.setTextColor(OLED_COLOR_BLACK);
    } else {
      _oled.setTextColor(OLED_COLOR_WHITE);
    }
    _oled.setCursor(4, y);
    _oled.print(items[idx]);
    y += ITEM_HEIGHT;
  }
  _oled.setTextColor(OLED_COLOR_WHITE);

  if (scrollOffset > 0) {
    _oled.setCursor(120, HEADER_HEIGHT + 2);
    _oled.print(F("^"));
  }
  if (scrollOffset + MENU_VISIBLE_ITEMS < count) {
    _oled.setCursor(120, HEADER_HEIGHT + 2 + (MENU_VISIBLE_ITEMS - 1) * ITEM_HEIGHT);
    _oled.print(F("v"));
  }
}

void display_centerText(const char* text, int16_t y, uint8_t textSize) {
  if (!_oledOk) return;
  _oled.setTextSize(textSize);
  int16_t x1, y1;
  uint16_t tw, th;
  _oled.getTextBounds(text, 0, 0, &x1, &y1, &tw, &th);
  _oled.setCursor((SCREEN_WIDTH - tw) / 2, y);
  _oled.print(text);
}

void display_confirmDialog(const char* title, const char* line1, const char* line2) {
  if (!_oledOk) return;
  _oled.clearDisplay();
  _oled.drawRect(4, 4, 120, 56, OLED_COLOR_WHITE);
  _oled.fillRect(4, 4, 120, 14, OLED_COLOR_WHITE);
  _oled.setTextColor(OLED_COLOR_BLACK);
  _oled.setTextSize(1);
  int16_t x1, y1;
  uint16_t tw, th;
  _oled.getTextBounds(title, 0, 0, &x1, &y1, &tw, &th);
  _oled.setCursor((SCREEN_WIDTH - tw) / 2, 7);
  _oled.print(title);
  _oled.setTextColor(OLED_COLOR_WHITE);

  _oled.setCursor(10, 22);
  _oled.print(line1);
  _oled.setCursor(10, 34);
  _oled.print(line2);

  _oled.setCursor(14, 48);
  _oled.print(F("[SEL] Confirm"));
  _oled.setCursor(80, 48);
  _oled.print(F("[<] No"));
  _oled.display();
}

void display_progress(const char* label, uint8_t pct) {
  if (!_oledOk) return;
  _oled.clearDisplay();
  display_centerText(label, 20, 1);
  display_barGraph(10, 36, 108, 12, pct);

  char buf[6];
  snprintf(buf, sizeof(buf), "%d%%", pct);
  display_centerText(buf, 52, 1);
  _oled.display();
}
