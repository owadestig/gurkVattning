#ifndef HTTPHANDLER_H
#define HTTPHANDLER_H

#include <ESP8266HTTPClient.h>
#include "WiFiManager.h"
#include "../lib/ESP8266Ping-master/src/ESP8266Ping.h"

String sendRequestToServer(const char *serverUrl);

#endif
