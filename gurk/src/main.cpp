// main.cpp

#include <ESP8266WiFi.h>
#include "../lib/ESP8266Ping-master/src/ESP8266Ping.h"
#include <WiFiClient.h>
#include "../lib/ArduinoJson-v6.21.5.h"
#include <ESP8266HTTPClient.h>
#include "WiFiManager.h"
#include "HTTPHandler.h"
#include "LEDHandler.h"
#include "Config.h"

const char *ssid = "WiFi";
const char *password = "7A62B947C2";
const char *serverUrl = "http://192.168.0.108:5001/data";
const char *noButtonSignalUrl = "http://192.168.0.108:5001/no_button_signal";
const char *logLightStatusUrl = "http://192.168.0.108:5001/log_light_status";
const int pinLED = 4;                          // Pin for output to light
const int pinInput = 12;                       // Pin for input signal
const int waitThreshold = 10000;               // Threshold for waiting or sleeping (in milliseconds)
const unsigned long maxOnDuration = 10000;     // Maximum duration for LED to stay on (in milliseconds)
const unsigned long reconnectInterval = 5000;  // Interval for attempting to reconnect (in milliseconds)
const unsigned long reconnectTimeout = 60000;  // Maximum time to attempt reconnection (in milliseconds)
const unsigned long standbyDuration = 7200000; // Duration for standby mode (in milliseconds, 2 hours)

void setup()
{
  // This is part of power saving
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();
  delay(1);
  WiFi.forceSleepWake();
  delay(1);
  // Disable the WiFi persistence.  The ESP8266 will not load and save WiFi settings in the flash memory.
  WiFi.persistent(false);
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  connectToWiFi(ssid, password);

  pinMode(pinLED, OUTPUT);
  pinMode(pinInput, INPUT_PULLUP); // Enable internal pull-up resistor
  Serial.begin(115200);
  delay(10);
  Serial.println("SDK version: " + String(ESP.getSdkVersion()));
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
}
