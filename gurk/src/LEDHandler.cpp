#include "LEDHandler.h"

extern int pinLED;
extern int pinInput;
extern const char *logLightStatusUrl;

void logLightStatus(const char *status)
{
    if (WiFi.status() == WL_CONNECTED)
    {
        WiFiClient client;
        HTTPClient http;
        http.begin(client, logLightStatusUrl);
        http.addHeader("Content-Type", "application/json");

        StaticJsonDocument<200> doc;
        doc["status"] = status;
        doc["timestamp"] = String(millis());

        String payload;
        serializeJson(doc, payload);

        int httpCode = http.POST(payload);
        if (httpCode > 0)
        {
            String response = http.getString();
            Serial.println(response);
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
        Serial.println("WiFi Disconnected during logging");
    }
}

void handleLED(int value, int maxOnDuration, const char *noButtonSignalUrl, const char *ssid, const char *password, int waitState)
{
    while (value > 0)
    {
        int interval = 1000;
        value -= interval;
        delay(interval);
    }
    Serial.printf("LED ON");
    digitalWrite(pinLED, HIGH);
    logLightStatus("on");
    disconnectFromWiFi();
    unsigned long startTime = millis();
    bool ButtonSignal = false;
    while (millis() - startTime < maxOnDuration)
    {
        if (digitalRead(pinInput) == waitState)
        {
            ButtonSignal = true;
            break;
        }
        delay(10); // Small delay to prevent high CPU usage
    }
    digitalWrite(pinLED, LOW); // Turn off the LED
    connectToWiFi(ssid, password);
    logLightStatus("off");
    if (!ButtonSignal)
    {
        sendRequestToServer(noButtonSignalUrl);
        shutdown("No button signal received, shutting down");
    }
}

void processResponse(const String &payload)
{
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error)
    {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.f_str());
        return;
    }

    int value = doc["value"];                 // seconds until turn on time
    int temp_val = doc["led_duration"];       // Access the value as an integer
    int time_interval = doc["time_interval"]; //
    unsigned long ledGap = temp_val * 1000;   // Multiply the retrieved value by 1000
    String current_time = doc["current_time"];

    if (value * 1000 < waitThreshold + 15000)
    {
        handleLED(value, maxOnDuration, noButtonSignalUrl, ssid, password, HIGH);
        delay(ledGap);
        handleLED(value, maxOnDuration, noButtonSignalUrl, ssid, password, LOW);
        return;
    }
    else
    {
        go_to_sleep(waitThreshold * 1000);
        return;
    }
}

void resetLED()
{
    if (digitalRead(pinInput) == HIGH)
    {
        handleLED(0, maxOnDuration, noButtonSignalUrl, ssid, password, LOW);
    }
}
