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
const char *constantsUrl = "http://192.168.0.108:5001/constants";

// Define variables to hold the constants fetched from the server
int pinLED;
int pinInput;
int waitThreshold;
unsigned long maxOnDuration;
unsigned long reconnectInterval;
unsigned long reconnectTimeout;
unsigned long standbyDuration;

void fetchConstants()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    WiFiClient client;
    HTTPClient http;
    http.begin(client, constantsUrl);
    int httpCode = http.GET();

    if (httpCode > 0)
    {
      String payload = http.getString();
      Serial.println(httpCode);
      Serial.println(payload);

      StaticJsonDocument<200> doc;
      DeserializationError error = deserializeJson(doc, payload);

      if (error)
      {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.f_str());
        return;
      }

      pinLED = doc["pinLED"];
      pinInput = doc["pinInput"];
      waitThreshold = doc["waitThreshold"];
      maxOnDuration = doc["maxOnDuration"];
      reconnectInterval = doc["reconnectInterval"];
      reconnectTimeout = doc["reconnectTimeout"];
      standbyDuration = doc["standbyDuration"];
    }
    else
    {
      Serial.print("Error on HTTP request: ");
      Serial.println(http.errorToString(httpCode).c_str());
    }
    http.end();
  }
  else
  {
    Serial.println("WiFi Disconnected");
    reconnectWiFi();
  }
}

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
  WiFi.mode(WIFI_STA);
  connectToWiFi(ssid, password);

  fetchConstants(); // Fetch constants from the server

  pinMode(pinLED, OUTPUT);
  pinMode(pinInput, INPUT_PULLUP); // Enable internal pull-up resistor
  Serial.begin(115200);
  delay(10);
  resetLED();
  if (!check_if_server_is_up(serverUrl))
  {
    offlineMode();
  }
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
