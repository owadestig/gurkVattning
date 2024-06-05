#include <ESP8266WiFi.h>
#include "../lib/ESP8266Ping-master/src/ESP8266Ping.h"
#include <WiFiClient.h>
#include "../lib/ArduinoJson-v6.21.5.h"
#include <ESP8266HTTPClient.h>

const char *ssid = "WiFi";
const char *password = "7A62B947C2";
const char *serverUrl = "http://192.168.0.108:5001/data";
const char *noSignalUrl = "http://192.168.0.108:5001/no_signal";
const int pinLED = 4;
const int pinInput = 12; // Pin for input signal
bool ledState = LOW;
const int waitThreshold = 10;                  // Threshold for waiting or sleeping (in seconds)
const unsigned long maxOnDuration = 5000;      // Maximum duration for LED to stay on (in milliseconds)
const unsigned long reconnectInterval = 5000;  // Interval for attempting to reconnect (in milliseconds)
const unsigned long reconnectTimeout = 60000;  // Maximum time to attempt reconnection (in milliseconds)
const unsigned long standbyDuration = 7200000; // Duration for standby mode (in milliseconds, 2 hours)

void standbyMode()
{
  Serial.printf("Entering standby mode for %d seconds...\n", standbyDuration / 1000);
  digitalWrite(pinLED, LOW); // Turn off LED
  delay(standbyDuration);    // Wait for the standby duration

  // After the standby duration, attempt to reconnect for 1 minute with 1-second intervals
  Serial.println("Attempting to reconnect after standby...");
  unsigned long startTime = millis();
  while (millis() - startTime < reconnectTimeout)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println("Reconnected to WiFi after standby!");
      return;
    }
    delay(1000); // Wait for 1 second before checking again
  }

  // If reconnection attempt fails after 1 minute, enter standby mode again
  Serial.println("Failed to reconnect after standby. Entering standby mode again.");
  standbyMode();
}

void reconnectWiFi()
{
  Serial.println("Attempting to reconnect to WiFi...");
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(reconnectInterval);
    Serial.print(".");
    if (millis() - startTime > reconnectTimeout)
    {
      Serial.println("\nFailed to reconnect to WiFi within the timeout period. Entering standby mode.");
      standbyMode();
      startTime = millis();
    }
  }
  Serial.println("\nReconnected to WiFi!");
}

void setup()
{
  pinMode(pinLED, OUTPUT);
  pinMode(pinInput, INPUT_PULLUP); // Enable internal pull-up resistor
  Serial.begin(115200);
  delay(10);
  Serial.println("SDK version: " + String(ESP.getSdkVersion()));

  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    if (millis() - startTime > reconnectTimeout)
    {
      Serial.println("\nFailed to connect to WiFi within the timeout period. Entering standby mode.");
      standbyMode();
      startTime = millis();
    }
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  IPAddress so(192, 168, 0, 108);
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

void sendRequestToServer(const char *url)
{
  if (WiFi.status() == WL_CONNECTED)
  {
    WiFiClient client;
    HTTPClient http;
    http.begin(client, url);
    int httpCode = http.GET();
    if (httpCode > 0)
    {
      String payload = http.getString();
      Serial.println(httpCode);
      Serial.println(payload);
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
      StaticJsonDocument<200> doc;
      DeserializationError error = deserializeJson(doc, payload);
      if (error)
      {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.f_str());
        return;
      }
      int value = doc["value"];
      unsigned long ledGap;
      int temp_val = doc["led_duration"]; // Access the value as an integer
      ledGap = temp_val * 1000;           // Multiply the retrieved value by 1000
      Serial.print("Value after JSON parsing = ");
      Serial.println(value);
      Serial.print("LED On Duration = ");
      Serial.println(ledGap);
      if (value < waitThreshold)
      {
        while (value > 0)
        {
          int interval = 500;
          value -= interval;
          delay(interval);
        }
        Serial.printf("Turning LED on, waiting for HIGH signal on pin %d or %d seconds\n", pinInput, maxOnDuration / 1000);
        digitalWrite(pinLED, HIGH);
        unsigned long startTime = millis();
        bool signalReceived = false;
        while (millis() - startTime < maxOnDuration)
        {
          if (digitalRead(pinInput) == HIGH)
          {
            // signalReceived = true;
            // break;
          }
          delay(10); // Small delay to prevent high CPU usage
        }
        digitalWrite(pinLED, LOW); // Turn off the LED
        if (!signalReceived)
        {
          Serial.println("No HIGH signal received for second light");
          sendRequestToServer(noSignalUrl); // Send a request to indicate no signal received
        }

        // Wait for the ledGap before turning the LED on again
        Serial.printf("Waiting %d seconds before turning LED on again\n", ledGap / 1000);
        delay(ledGap);

        // Turn the LED on again
        Serial.printf("Turning LED on again, waiting for HIGH signal on pin %d or %d seconds\n", pinInput, maxOnDuration / 1000);
        digitalWrite(pinLED, HIGH);
        startTime = millis();
        signalReceived = false;
        while (millis() - startTime < maxOnDuration)
        {
          if (digitalRead(pinInput) == HIGH)
          {
            // signalReceived = true;
            // break;
          }
          delay(10); // Small delay to prevent high CPU usage
        }
        digitalWrite(pinLED, LOW); // Turn off the LED
        if (!signalReceived)
        {
          Serial.println("No HIGH signal received for SECOND light");
          sendRequestToServer(noSignalUrl); // Send a request to indicate no signal received
        }
      }
      else
      {
        Serial.printf("Sleeping for %d seconds\n", waitThreshold);
        delay(waitThreshold * 1000); // Sleep for the threshold time
      }
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
