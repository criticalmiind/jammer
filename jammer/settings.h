#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>

struct AppSettings {
  char ap_ssid[33];
  char ap_pass[65];
  bool ap_on;
  uint8_t ap_channel;
  uint32_t magic;
};

extern AppSettings globalSettings;

void settings_init();
void settings_save();
void settings_reset();

#endif // SETTINGS_H
