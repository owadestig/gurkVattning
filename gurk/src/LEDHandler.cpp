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

void handleLED(int time_until_watering, int maxOnDuration, const char *noButtonSignalUrl, const char *ssid, const char *password, int waitState)
{
    disconnectFromWiFi();
    while (time_until_watering > 0)
    {
        int interval = 1000;
        time_until_watering -= interval;
        delay(interval);
    }
    Serial.printf("LED ON");
    digitalWrite(pinLED, HIGH);
    connectToWiFi(ssid, password);
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

    int time_until_watering = 1000 * int(doc["time_until_watering"]); // seconds until turn on time, turned into milliseconds
    unsigned long watering_time = doc["led_duration"];
    String current_time = doc["current_time"];

    if (time_until_watering * 1000 < waitThreshold + 15000)
    {
        if (time_until_watering * 10000 < 20000)
        {
            handleLED(time_until_watering, maxOnDuration, noButtonSignalUrl, ssid, password, HIGH);
            delay(watering_time);
            handleLED(time_until_watering, maxOnDuration, noButtonSignalUrl, ssid, password, LOW);
            return;
        }
        else
        {
            go_to_sleep(time_until_watering - 20 * 1000);
        }
    }
    else
    {
        go_to_sleep(waitThreshold);
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

void offlineMode()
{
    disconnectFromWiFi();
    int watering_time = 1000 * 60 * 5; // 5 minutes, default
    handleLEDDisconnected(HIGH);
    delay(watering_time);
    handleLEDDisconnected(LOW);
    int sleep_time = 1000 * 60 * 60 * 24; // 1 day
    go_to_sleep(sleep_time);
}

void handleLEDDisconnected(int waitState)
{
    digitalWrite(pinLED, HIGH);
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
    if (!ButtonSignal)
    {
        shutdown("No button signal received when DC'd, shutting down");
    }
}