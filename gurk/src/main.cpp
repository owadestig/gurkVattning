#include <ESP8266WiFi.h>
#include "../lib/ESP8266Ping-master/src/ESP8266Ping.h"
#include <WiFiClient.h>
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
const int waitThreshold = 1000 * 60 * 240;
const unsigned long maxOnDuration = 10000;
const unsigned long reconnectInterval = 5000;
const unsigned long reconnectTimeout = 60000;
const unsigned long standbyDuration = 7200000;
// temp

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
  connectToWiFi(ssid, password);

  pinMode(pinLED, OUTPUT);
  pinMode(pinInput, INPUT_PULLUP); // Enable internal pull-up resistor

  Serial.println("Setup complete");
  resetLED();
}

void loop()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    WiFiClient client;
    HTTPClient http;
    http.begin(client, serverUrl);
    int httpCode = http.GET();

    if (httpCode > 0)
    {
      String payload = http.getString();
      Serial.println(httpCode);
      Serial.println(payload);
      processResponse(payload); // Call the function with the correct parameters
    }
    else
    {
      Serial.print("Error on HTTP request: ");
      Serial.println(http.errorToString(httpCode).c_str());
      digitalWrite(pinLED, LOW);
    }
    http.end();
  }
  else
  {
    Serial.println("WiFi Disconnected");
    digitalWrite(pinLED, LOW);
    reconnectWiFi();
  }
  delay(5000); // Add a delay to reduce the serial output frequency
}
