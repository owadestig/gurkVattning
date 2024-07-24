#include <ESP8266WiFi.h>
#include "../lib/ESP8266Ping-master/src/ESP8266Ping.h"
#include <WiFiClientSecure.h>
#include "../lib/ArduinoJson-v6.21.5.h"
#include <ESP8266HTTPClient.h>
#include "WiFiManager.h"
#include "HTTPHandler.h"
#include "LEDHandler.h"
#include "Config.h"

const char *ssid = "Eagle_389AD0";
const char *password = "CiKbPq6b";
const char *serverUrl = "https://gurkvattning.onrender.com/data";
const char *noButtonSignalUrl = "https://gurkvattning.onrender.com/no_button_signal";
const char *logLightStatusUrl = "https://gurkvattning.onrender.com/log_light_status";
const char *constantsUrl = "https://gurkvattning.onrender.com/constants";

// Define variables to hold the constants fetched from the server
const int pinLED = 5;
const int pinInput = 14;
const int waitThreshold = 1000 * 60 * 1; // 1 timme
const unsigned long maxOnDuration = 10000; // not used
const unsigned long reconnectInterval = 5000; // not used
const unsigned long reconnectTimeout = 60000; // not used
const unsigned long standbyDuration = 7200000; // not used

void setup()
{
  Serial.begin(115200);
  delay(10);

  // This is part of power saving
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();
  delay(1);
  WiFi.forceSleepWake();
  delay(1);

  // Disable the WiFi persistence. The ESP8266 will not load and save WiFi settings in the flash memory.
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  Serial.println("I setup");
  connectToWiFi(ssid, password);
  pinMode(pinLED, OUTPUT);
  pinMode(pinInput, INPUT_PULLUP); // Enable internal pull-up resistor
  Serial.println("Setup complete");
  resetLED();
}

void loop()
{
  int counter = 0;
  if (WiFi.status() == WL_CONNECTED)
  {
    WiFiClientSecure client;
    client.setInsecure(); // Disable SSL certificate verification

    HTTPClient https;

    Serial.print("[HTTPS] begin...\n");
    if (https.begin(client, serverUrl))
    {
      Serial.print("[HTTPS] GET...\n");
      int httpCode = https.GET();

      if (httpCode > 0)
      {
        Serial.printf("[HTTPS] GET... code: %d\n", httpCode);

        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
        {
          String payload = https.getString();
          Serial.println(payload);
          processResponse(payload); // Call the function with the correct parameters
        }
      }
      else
      {
        Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }

      https.end();
    }
    else
    {
      Serial.printf("[HTTPS] Unable to connect\n");
      delay(20000);
    }
  }
  else
  {
    Serial.println("WiFi Disconnected");
    delay(20000);
    reconnectWiFi();
  }
  delay(1000); // Add a delay to reduce the serial output frequency
}