#include <ESP8266WiFi.h>
#include "../lib/ESP8266Ping-master/src/ESP8266Ping.h"
#include <WiFiClient.h>
#include "../lib/ArduinoJson-v6.21.5.h"
#include <ESP8266HTTPClient.h>

// Replace with your network credentials
const char *ssid = "WiFi";
const char *password = "7A62B947C2";

const char *serverUrl = "http://192.168.0.108:5001/data"; // Change this to your local IP and port

const int pin = 4;
bool ledState = LOW;

void setup()
{
  pinMode(pin, OUTPUT);
  Serial.begin(115200);
  delay(10);
  Serial.println("SDK version: " + String(ESP.getSdkVersion()));

  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  IPAddress so(192, 168, 0, 108); // The remote IP to ping
  bool ret = Ping.ping(so);
  if (ret)
  {
    int avg_time_ms = Ping.averageTime();
    Serial.print("Average time = ");
    Serial.println(avg_time_ms);
  }
  else
  {
    Serial.println("Couldn't find host...");
  }
}

void loop()
{
  if (WiFi.status() == WL_CONNECTED)
  {                    // Check if connected to the WiFi network
    WiFiClient client; // Create a WiFiClient object
    HTTPClient http;

    http.begin(client, serverUrl); // Use the new API with WiFiClient
    int httpCode = http.GET();     // Make the GET request

    if (httpCode > 0)
    {                                    // Check for the returning code
      String payload = http.getString(); // Get the request response payload

      Serial.println(httpCode); // Print HTTP return code
      Serial.println(payload);  // Print request response payload

      // Parse JSON
      StaticJsonDocument<200> doc;
      DeserializationError error = deserializeJson(doc, payload);

      if (error)
      {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.f_str());
        return;
      }

      int value = doc["value"]; // Extract the value from the JSON object
      Serial.print("Value after JSON parsing = ");
      Serial.println(value);

      if (value % 5 == 1)
      {
        Serial.println("ON");
        digitalWrite(pin, HIGH); // Turn on the LED
      }
      else
      {
        Serial.println("OFF");
        digitalWrite(pin, LOW); // Turn off the LED
      }
    }
    else
    {
      Serial.print("Error on HTTP request: ");
      Serial.println(http.errorToString(httpCode).c_str());
      digitalWrite(pin, LOW); // Ensure the LED is off if there's an error
    }

    http.end(); // Free the resources
  }
  else
  {
    Serial.println("WiFi Disconnected");
    digitalWrite(pin, LOW); // Ensure the LED is off if disconnected
  }

  delay(2000); // Delay between each request
}
