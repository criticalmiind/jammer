#include "settings.h"
#include <EEPROM.h>
#include <string.h>

#define SETTINGS_MAGIC 0xDECAFBAD
AppSettings globalSettings;

void settings_save() {
  globalSettings.magic = SETTINGS_MAGIC;
  EEPROM.begin(sizeof(AppSettings));
  EEPROM.put(0, globalSettings);
  EEPROM.commit();
}

void settings_reset() {
  strncpy(globalSettings.ap_ssid, "ShadowNet_Panel", 32);
  globalSettings.ap_ssid[32] = '\0';
  globalSettings.ap_pass[0] = '\0'; // Open by default
  globalSettings.ap_on = true;
  globalSettings.ap_channel = 1;
  globalSettings.magic = SETTINGS_MAGIC;
  settings_save();
}

void settings_init() {
  EEPROM.begin(sizeof(AppSettings));
  EEPROM.get(0, globalSettings);
  if (globalSettings.magic != SETTINGS_MAGIC) {
    settings_reset();
  }
}
