/*
 * battery_manager.h — Battery voltage reading & percentage
 */

#ifndef BATTERY_MANAGER_H
#define BATTERY_MANAGER_H

#include <Arduino.h>
#include "config.h"

// Initialize the ADC pin for battery reading
void battery_init();

// Non-blocking update — call every loop iteration.
// Internally reads only every BATT_READ_INTERVAL_MS.
void battery_update();

// Get the last computed battery percentage (0-100)
uint8_t battery_getPercent();

// Get the last computed battery voltage (e.g., 3.85)
float battery_getVoltage();

// Returns true if battery voltage is below BATT_LOW_WARN
bool battery_isLow();

#endif // BATTERY_MANAGER_H
