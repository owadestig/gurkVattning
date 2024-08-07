#ifndef WIFIMANAGER_H
#define WIFIMANAGER_H

#include <ESP8266WiFi.h>
#include "Utils.h"

void connectToWiFi(const char *ssid, const char *password);
void disconnectFromWiFi();

#endif
