/*
 * buttons.h — Debounced button input handling
 */

#ifndef BUTTONS_H
#define BUTTONS_H

#include <Arduino.h>
#include "config.h"

// Button event types
enum ButtonEvent {
  BTN_NONE = 0,
  BTN_EVT_UP,
  BTN_EVT_DOWN,
  BTN_EVT_SELECT,
  BTN_EVT_BACK
};

// Initialize all button pins with internal pull-ups
void buttons_init();

// Poll buttons and return the first detected press event.
// Uses debouncing — returns BTN_NONE if no new press detected.
ButtonEvent buttons_poll();

#endif // BUTTONS_H
