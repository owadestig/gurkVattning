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
const char *serverUrl = "https://gurkvattning.onrender.com/get_device_variables";
const char *noButtonSignalUrl = "https://gurkvattning.onrender.com/no_button_signal";
const char *set_is_watering_rul = "https://gurkvattning.onrender.com/set_is_watering";

// Define variables to hold the constants fetched from the server
const int pinLED = 5;
const int pinInput = 14;
const unsigned long maxOnDuration = 10000;
const int errorTimeout = 20000; // 20 sekunder

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
  if (WiFi.status() == WL_CONNECTED)
  {
    String payload = sendRequestToServer(serverUrl);
    if (payload != "error")
    {
      processResponse(payload);
    }
    else
    {
      disconnectFromWiFi();
      delay(errorTimeout);
      connectToWiFi(ssid, password);
      Serial.println("Error");
    }
  }
  else
  {
    Serial.println("WiFi Disconnected");
    delay(errorTimeout);
    connectToWiFi(ssid, password);
  }
  delay(1000); // Add a delay to reduce the serial output frequency
}