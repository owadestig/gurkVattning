#ifndef HTTPHANDLER_H
#define HTTPHANDLER_H

#include <ESP8266HTTPClient.h>
#include "WifiManager.h"

String sendHttpRequest(const char *url);
void sendRequestToServer(const char *url);

#endif
