/*
 * display_manager.cpp — OLED display driver implementation
 *
 * Wraps Adafruit_SSD1306 for the 128×64 OLED, providing higher-level
 * drawing helpers used throughout the menu system.
 */

#include "display_manager.h"

// Global display object
static Adafruit_SSD1306 _oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

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

/*
 * display_init()
 * Start I2C and initialize the OLED. Returns false if the display
 * is not detected on the bus.
 */
bool display_init() {
  Wire.begin(SDA_PIN, SCL_PIN);
  if (!_oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    DBGLN(F("[OLED] SSD1306 init FAILED"));
    return false;
  }
  _oled.setTextColor(SSD1306_WHITE);
  _oled.setTextSize(1);
  _oled.cp437(true);
  _oled.clearDisplay();
  _oled.display();
  DBGLN(F("[OLED] Display initialized"));
  return true;
}

Adafruit_SSD1306& display_get() {
  return _oled;
}

void display_clear() {
  _oled.clearDisplay();
}

void display_update() {
  _oled.display();
}

/*
 * display_setBrightness()
 * Adjusts the OLED contrast register (0 = dimmest, 255 = brightest).
 */
void display_setBrightness(uint8_t brightness) {
  _oled.ssd1306_command(SSD1306_SETCONTRAST);
  _oled.ssd1306_command(brightness);
}

/*
 * display_splash()
 * Boot screen with project name, version, disclaimer.
 */
void display_splash() {
  _oled.clearDisplay();

  // Title
  _oled.setTextSize(1);
  _oled.setCursor(10, 4);
  _oled.print(F("WIRELESS RESEARCH"));

  _oled.setCursor(40, 16);
  _oled.setTextSize(1);
  _oled.print(F("TOOL"));

  // Version
  _oled.setTextSize(1);
  _oled.setCursor(36, 30);
  _oled.print(F("v"));
  _oled.print(F(FW_VERSION));

  // Disclaimer
  _oled.setCursor(4, 46);
  _oled.setTextSize(1);
  _oled.print(F("Educational Use Only"));

  _oled.setCursor(16, 56);
  _oled.print(F("Lab Testing Only"));

  _oled.display();
}

/*
 * display_statusBar()
 * Draws a 12-pixel-high bar at the top with battery percentage
 * and the current mode label (right-aligned).
 */
void display_statusBar(uint8_t battPct, const char* modeLabel) {
  // Background
  _oled.fillRect(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, SSD1306_WHITE);

  // Battery icon
  _oled.drawBitmap(2, 3, _battIcon, 12, 6, SSD1306_BLACK);

  // Fill proportional to percentage
  uint8_t fillW = map(constrain(battPct, 0, 100), 0, 100, 0, 8);
  _oled.fillRect(4, 4, fillW, 4, SSD1306_BLACK);

  // Percentage text
  char buf[6];
  snprintf(buf, sizeof(buf), "%d%%", battPct);
  _oled.setTextSize(1);
  _oled.setTextColor(SSD1306_BLACK);
  _oled.setCursor(16, 2);
  _oled.print(buf);

  // Mode label (right-aligned)
  int16_t x1, y1;
  uint16_t tw, th;
  _oled.getTextBounds(modeLabel, 0, 0, &x1, &y1, &tw, &th);
  _oled.setCursor(SCREEN_WIDTH - tw - 4, 2);
  _oled.print(modeLabel);

  // Reset text color for subsequent drawing
  _oled.setTextColor(SSD1306_WHITE);
}

/*
 * display_barGraph()
 * Draws a horizontal bar with border (like a progress bar).
 */
void display_barGraph(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t pct) {
  _oled.drawRect(x, y, w, h, SSD1306_WHITE);
  int16_t fillW = map(constrain(pct, 0, 100), 0, 100, 0, w - 2);
  _oled.fillRect(x + 1, y + 1, fillW, h - 2, SSD1306_WHITE);
}

/*
 * display_vertBar()
 * Draws a single vertical bar for spectrum / channel graphs.
 * x       = left edge of bar
 * bottom  = y coordinate of the baseline
 * maxH    = maximum height in pixels
 * value   = current reading
 * maxVal  = value that would produce maxH
 */
void display_vertBar(int16_t x, int16_t bottom, int16_t maxH, uint8_t value, uint8_t maxVal) {
  if (maxVal == 0) return;
  int16_t h = map(constrain(value, 0, maxVal), 0, maxVal, 0, maxH);
  if (h > 0) {
    _oled.fillRect(x, bottom - h, 1, h, SSD1306_WHITE);
  }
}

/*
 * display_list()
 * Renders a scrollable list of text items.
 * Highlighted item is drawn inverse (white bg, black text).
 */
void display_list(const char* items[], uint8_t count, uint8_t selected, uint8_t scrollOffset) {
  int16_t y = HEADER_HEIGHT + 2;
  for (uint8_t i = 0; i < MENU_VISIBLE_ITEMS && (scrollOffset + i) < count; i++) {
    uint8_t idx = scrollOffset + i;
    if (idx == selected) {
      _oled.fillRect(0, y - 1, SCREEN_WIDTH, ITEM_HEIGHT, SSD1306_WHITE);
      _oled.setTextColor(SSD1306_BLACK);
    } else {
      _oled.setTextColor(SSD1306_WHITE);
    }
    _oled.setCursor(4, y);
    _oled.print(items[idx]);
    y += ITEM_HEIGHT;
  }
  _oled.setTextColor(SSD1306_WHITE);

  // Scroll indicators
  if (scrollOffset > 0) {
    _oled.setCursor(120, HEADER_HEIGHT + 2);
    _oled.print(F("^"));
  }
  if (scrollOffset + MENU_VISIBLE_ITEMS < count) {
    _oled.setCursor(120, HEADER_HEIGHT + 2 + (MENU_VISIBLE_ITEMS - 1) * ITEM_HEIGHT);
    _oled.print(F("v"));
  }
}

/*
 * display_centerText()
 * Prints text horizontally centered at the given y coordinate.
 */
void display_centerText(const char* text, int16_t y, uint8_t textSize) {
  _oled.setTextSize(textSize);
  int16_t x1, y1;
  uint16_t tw, th;
  _oled.getTextBounds(text, 0, 0, &x1, &y1, &tw, &th);
  _oled.setCursor((SCREEN_WIDTH - tw) / 2, y);
  _oled.print(text);
}

/*
 * display_confirmDialog()
 * Draws a modal confirmation box.  The caller handles button input.
 */
void display_confirmDialog(const char* title, const char* line1, const char* line2) {
  _oled.clearDisplay();
  // Border
  _oled.drawRect(4, 4, 120, 56, SSD1306_WHITE);
  // Title bar
  _oled.fillRect(4, 4, 120, 14, SSD1306_WHITE);
  _oled.setTextColor(SSD1306_BLACK);
  _oled.setTextSize(1);
  int16_t x1, y1;
  uint16_t tw, th;
  _oled.getTextBounds(title, 0, 0, &x1, &y1, &tw, &th);
  _oled.setCursor((SCREEN_WIDTH - tw) / 2, 7);
  _oled.print(title);
  _oled.setTextColor(SSD1306_WHITE);

  // Body text
  _oled.setCursor(10, 22);
  _oled.print(line1);
  _oled.setCursor(10, 34);
  _oled.print(line2);

  // Buttons
  _oled.setCursor(14, 48);
  _oled.print(F("[SEL] Confirm"));
  _oled.setCursor(80, 48);
  _oled.print(F("[<] No"));
  _oled.display();
}

/*
 * display_progress()
 * Full-screen progress bar with a label.
 */
void display_progress(const char* label, uint8_t pct) {
  _oled.clearDisplay();
  display_centerText(label, 20, 1);
  display_barGraph(10, 36, 108, 12, pct);

  char buf[6];
  snprintf(buf, sizeof(buf), "%d%%", pct);
  display_centerText(buf, 52, 1);
  _oled.display();
}
