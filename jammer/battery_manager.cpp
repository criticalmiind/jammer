/*
 * battery_manager.cpp — Battery voltage monitoring
 *
 * Reads battery voltage through a 100k/100k resistor divider
 * on GPIO34 (ADC1_CH6).  Averages BATT_SAMPLES readings for
 * stability.  Non-blocking — uses millis() timer.
 */

#include "battery_manager.h"

static float    _voltage  = 0.0;
static uint8_t  _percent  = 0;
static unsigned long _lastRead = 0;

/*
 * battery_init()
 * Configure the ADC pin.  ADC1 channel is used because ADC2
 * conflicts with WiFi on ESP32.
 */
void battery_init() {

  pinMode(BATTERY_PIN, INPUT);
  battery_update();                    // Take an initial reading
  DBGLN(F("[BATT] Battery manager initialized"));
}

/*
 * battery_update()
 * Called every loop().  Only performs an actual ADC read every
 * BATT_READ_INTERVAL_MS to avoid wasting CPU.
 */
void battery_update() {
  unsigned long now = millis();
  if (now - _lastRead < BATT_READ_INTERVAL_MS && _lastRead != 0) return;
  _lastRead = now;

  // Average multiple samples for a stable reading
  uint32_t sum = 0;
  for (uint8_t i = 0; i < BATT_SAMPLES; i++) {
    sum += analogRead(BATTERY_PIN);
  }
  float adcAvg = (float)sum / BATT_SAMPLES;

  // Convert ADC to voltage (10-bit, 3.3V reference)
  float pinVoltage = (adcAvg / 1023.0) * 3.3;

  // Account for the resistor divider (1:1 → multiply by 2)
  _voltage = pinVoltage * ((BATT_R1 + BATT_R2) / BATT_R2);

  // Map voltage range to 0-100%
  float pctF = ((_voltage - BATT_MIN_V) / (BATT_MAX_V - BATT_MIN_V)) * 100.0;
  _percent = (uint8_t)constrain(pctF, 0, 100);

  DBGF("[BATT] %.2fV  %d%%\n", _voltage, _percent);
}

uint8_t battery_getPercent() {
  return _percent;
}

float battery_getVoltage() {
  return _voltage;
}

bool battery_isLow() {
  return _voltage < BATT_LOW_WARN && _voltage > 0.5; // >0.5 avoids false alarm when USB powered
}
