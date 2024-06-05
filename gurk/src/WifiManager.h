#ifndef WIFIMANAGER_H
#define WIFIMANAGER_H

#include <ESP8266WiFi.h>
#include "Utils.h"

void standbyMode(unsigned long standbyDuration, unsigned long reconnectTimeout);
void reconnectWiFi();
void connectToWiFi(const char *ssid, const char *password);
void disconnectFromWiFi();

#endif
