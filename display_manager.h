/*
 * display_manager.h — OLED display driver wrapper
 *
 * Provides helper functions for the 128×64 SSD1306/SH1106 OLED
 * including splash screen, status bar, text, and graph drawing.
 */

#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"

// Initialize the OLED display. Returns false if not found.
bool display_init();

// Get a reference to the underlying Adafruit_SSD1306 object
Adafruit_SSD1306& display_get();

// Clear the entire display buffer
void display_clear();

// Push the buffer to the screen
void display_update();

// Set display contrast/brightness (0-255)
void display_setBrightness(uint8_t brightness);

// Show the boot splash screen
void display_splash();

// Draw the top status bar (battery %, WiFi icon, mode label)
void display_statusBar(uint8_t battPct, const char* modeLabel);

// Draw a horizontal bar graph (x, y, width, height, value 0-100)
void display_barGraph(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t pct);

// Draw a vertical bar at a given x position (for spectrum graphs)
void display_vertBar(int16_t x, int16_t bottom, int16_t maxH, uint8_t value, uint8_t maxVal);

// Draw a scrollable text list (used for scan results)
// items[] is an array of C-strings, count is total items,
// selected is the highlighted index, scrollOffset is the first visible item.
void display_list(const char* items[], uint8_t count, uint8_t selected, uint8_t scrollOffset);

// Print centered text at a given y position
void display_centerText(const char* text, int16_t y, uint8_t textSize = 1);

// Draw a confirmation dialog box
// Returns nothing — the caller must handle button events.
void display_confirmDialog(const char* title, const char* line1, const char* line2);

// Draw a progress bar with label
void display_progress(const char* label, uint8_t pct);

#endif // DISPLAY_MANAGER_H
