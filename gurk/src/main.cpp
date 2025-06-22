#include <ESP8266WiFi.h>
#include "../lib/ESP8266Ping-master/src/ESP8266Ping.h"
#include <WiFiClientSecure.h>
#include "ArduinoJson.h"
#include <ESP8266HTTPClient.h>
#include "WiFiManager.h"
#include "HTTPHandler.h"
#include "WaterController.h" // Updated include
#include "Config.h"

const char *ssid = "Eagle_389AD0";
const char *password = "CiKbPq6b";
// const char *serverUrl = "https://gurkvattning.onrender.com/get_device_variables";
// const char *noButtonSignalUrl = "https://gurkvattning.onrender.com/no_button_signal";
// const char *set_is_watering_rul = "https://gurkvattning.onrender.com/set_is_watering";
const char *serverUrl = "http://192.168.38.169:5001/get_device_variables";
const char *noButtonSignalUrl = "http://192.168.38.169:5001/no_button_signal";
const char *set_is_watering_rul = "http://192.168.38.169:5001/set_is_watering";

// Define variables to hold the constants fetched from the server
const int pinMotor = 16; // Actually controls the valve
const int pinInput = 5;  // Actually reads valve position sensor
const unsigned long maxOnDuration = 10000;
const int errorTimeout = 20000; // 20 sekunder

void setup()
{
  Serial.begin(115200);
  delay(10);

  // Initialize WiFiManager and power saving settings
  WiFiManager &wifiManager = WiFiManager::getInstance();
  wifiManager.initializePowerSaving();

  Serial.println("I setup");

  // Connect to WiFi using the WiFiManager
  if (wifiManager.connectToWiFi(ssid, password))
  {
    Serial.println("Successfully connected to WiFi");
    Serial.print("IP Address: ");
    Serial.println(wifiManager.getLocalIP());
  }
  else
  {
    Serial.println("Failed to connect to WiFi in setup");
  }

  // Initialize water controller
  WaterController &valveController = WaterController::getInstance();
  valveController.initialize(pinMotor, pinInput, maxOnDuration);
  valveController.setNetworkConfig(ssid, password, set_is_watering_rul, noButtonSignalUrl);

  Serial.println("Setup complete");
  valveController.resetValve();
}

void loop()
{
  WiFiManager &wifiManager = WiFiManager::getInstance();

  if (wifiManager.isConnected())
  {
    HTTPHandler &httpHandler = HTTPHandler::getInstance();
    String payload = httpHandler.sendRequest(serverUrl);
    if (payload != "error")
    {
      Serial.println(payload);
      WaterController::getInstance().processResponse(payload);
    }
    else
    {
      wifiManager.disconnectFromWiFi();
      delay(errorTimeout);
      wifiManager.connectToWiFi(ssid, password);
      Serial.println("Payload Error");
    }
  }
  else
  {
    Serial.println("WiFi Disconnected");
    delay(errorTimeout);
    wifiManager.connectToWiFi(ssid, password);
  }
  delay(1000);
}