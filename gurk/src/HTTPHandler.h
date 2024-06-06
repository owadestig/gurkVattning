#ifndef HTTPHANDLER_H
#define HTTPHANDLER_H

#include <ESP8266HTTPClient.h>
#include "WifiManager.h"
#include "../lib/ESP8266Ping-master/src/ESP8266Ping.h"

void sendRequestToServer(const char *url);
int check_if_server_is_up(const char *url);

#endif
