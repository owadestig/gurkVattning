#include "LEDHandler.h"

extern const int pinLED;
extern const int pinInput;
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
            Serial.println(httpCode);
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
        Serial.println("WiFi Disconnected");
    }
}

void handleLED(int value, unsigned long ledGap, int maxOnDuration, const char *noButtonSignalUrl, const char *ssid, const char *password, int waitState)
{
    // Debounce parameters
    unsigned long debounceDelay = 50;   // Debounce delay in milliseconds
    int lastButtonState = LOW;          // Previous button state
    int buttonState = LOW;              // Current button state
    unsigned long lastDebounceTime = 0; // Last time the button state was toggled

    while (value > 0)
    {
        int interval = 500;
        value -= interval;
        delay(interval);
    }

    Serial.printf("Turning LED on, waiting for HIGH signal on pin %d or %d seconds\n", pinInput, maxOnDuration / 1000);
    digitalWrite(pinLED, HIGH);
    logLightStatus("on");
    disconnectFromWiFi();
    unsigned long startTime = millis();
    bool ButtonSignal = false;

    while (millis() - startTime < maxOnDuration)
    {
        int reading = digitalRead(pinInput);

        if (reading != lastButtonState)
        {
            lastDebounceTime = millis(); // Reset the debouncing timer
        }

        if ((millis() - lastDebounceTime) > debounceDelay)
        {
            if (reading != buttonState)
            {
                buttonState = reading;

                if (buttonState == waitState)
                {
                    ButtonSignal = true;
                    break;
                }
            }
        }

        lastButtonState = reading;
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

    int value = doc["value"];
    int temp_val = doc["led_duration"]; // Access the value as an integer
    int time_interval = doc["time_interval"];
    unsigned long ledGap = temp_val * 1000; // Multiply the retrieved value by 1000
    String current_time = doc["current_time"];

    Serial.print("Value after JSON parsing = ");
    Serial.println(value);
    Serial.print("LED On Duration = ");
    Serial.println(ledGap);
    Serial.print("Time Interval = ");
    Serial.println(time_interval);
    Serial.print("Current Time = ");
    Serial.println(current_time);

    if (value < waitThreshold)
    {
        handleLED(value, ledGap, maxOnDuration, noButtonSignalUrl, ssid, password, HIGH);
        delay(ledGap);
        handleLED(value, ledGap, maxOnDuration, noButtonSignalUrl, ssid, password, LOW);
    }
    else
    {
        Serial.printf("Sleeping for %d seconds\n", waitThreshold);
        disconnectFromWiFi();
        delay(waitThreshold); // Sleep for the threshold time
        connectToWiFi(ssid, password);
    }
}
