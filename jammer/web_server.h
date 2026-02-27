#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "config.h"

// Initialize the ESP32 SoftAP and WebServer
void web_server_init();

// Handle client requests (should be called in loop)
void web_server_update();

#endif // WEB_SERVER_H
