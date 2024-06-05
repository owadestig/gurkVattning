// LEDHandler.h

#ifndef LED_HANDLER_H
#define LED_HANDLER_H

#include <Arduino.h>
#include "HTTPHandler.h"
#include "WiFiManager.h"
#include "Config.h"
#include "../lib/ArduinoJson-v6.21.5.h"
#include "Utils.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

void handleLED(int value, unsigned long ledGap);
void processResponse(const String &payload); // Corrected function signature

#endif // LED_HANDLER_H
