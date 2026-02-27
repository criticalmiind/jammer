/*
 * buttons.cpp — Debounced button input handling
 *
 * All six navigation buttons use internal pull-ups and are read
 * as active LOW.  A simple timestamp-based debounce prevents
 * repeated triggering within BTN_DEBOUNCE_MS.
 */

#include "buttons.h"

// Pin list matching the ButtonEvent enum order (index 0 unused)
static const uint8_t _pins[] = {
  BTN_UP, BTN_DOWN, BTN_SELECT, BTN_BACK
};
static const uint8_t _numBtns = sizeof(_pins) / sizeof(_pins[0]);

// Stores the last trigger time for each button
static unsigned long _lastPress[4] = {0};

/*
 * buttons_init()
 * Configure every button pin as INPUT_PULLUP.
 */
void buttons_init() {
  for (uint8_t i = 0; i < _numBtns; i++) {
    pinMode(_pins[i], INPUT_PULLUP);
  }
  DBGLN(F("[BTN] Buttons initialized"));
}

/*
 * buttons_poll()
 * Scans all buttons and returns the first new press detected.
 * Returns BTN_NONE if nothing pressed or still within debounce window.
 */
ButtonEvent buttons_poll() {
  unsigned long now = millis();
  for (uint8_t i = 0; i < _numBtns; i++) {
    if (digitalRead(_pins[i]) == LOW) {
      if (now - _lastPress[i] > BTN_DEBOUNCE_MS) {
        _lastPress[i] = now;
        return (ButtonEvent)(i + 1);  // BTN_EVT_UP = 1 ...
      }
    }
  }
  return BTN_NONE;
}
