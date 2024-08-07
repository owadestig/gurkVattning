#include "LEDHandler.h"

extern const int pinLED;
extern const int pinInput;
extern const char *set_is_watering_rul;

void sendWateringStatus(boolean status)
{
    if (WiFi.status() == WL_CONNECTED)
    {
        WiFiClient client;
        HTTPClient http;
        http.begin(client, set_is_watering_rul);
        http.addHeader("Content-Type", "application/json");

        StaticJsonDocument<200> doc;
        doc["isWatering"] = status;

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

void handleLED(int time_until_watering, int maxOnDuration, const char *noButtonSignalUrl, const char *ssid, const char *password, int waitState)
{
    // Turn on the LED
    digitalWrite(pinLED, HIGH);
    disconnectFromWiFi();
    unsigned long startTime = millis();
    bool ButtonSignal = false;

    while (millis() - startTime < maxOnDuration)
    {
        // Read the button state directly without debouncing
        int buttonState = digitalRead(pinInput);

        // Check if the button state matches the wait state
        if (buttonState == waitState)
        {
            ButtonSignal = true;
            break;
        }

        delay(10); // Small delay to prevent high CPU usage
    }

    // Turn off the LED
    digitalWrite(pinLED, LOW);
    connectToWiFi(ssid, password);

    // If no button signal received, perform shutdown
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

    int time_until_watering = 1000 * int(doc["time_until_watering"]);
    unsigned long watering_time = doc["watering_time"] * 1000 * 60; // Access the value as an integer
    int sleep_time = doc["sleep_time"] * 1000;
    Serial.print("time_until_watering after JSON parsing = ");
    Serial.println(time_until_watering);
    Serial.print("LED On Duration = ");
    Serial.println(watering_time);
    Serial.print("Sleep Time = ");
    Serial.println(sleep_time);

    if (time_until_watering < sleep_time + 20000) // 4 timmar + 15 sekunder
    {                                             // är här inne om mindre än sleep_time tid tills vattning
        if (time_until_watering > 0)              // om tiden är mer än 0, alltså vi ska vänta
        {
            disconnectFromWiFi();
            Serial.printf("Sleeping for %d ms before doing a cycle\n", time_until_watering);
            delay(time_until_watering);
            connectToWiFi(ssid, password);
        }

        handleLED(time_until_watering, maxOnDuration, noButtonSignalUrl, ssid, password, LOW);
        sendWateringStatus(true);
        disconnectFromWiFi();
        delay(watering_time);
        connectToWiFi(ssid, password);
        handleLED(time_until_watering, maxOnDuration, noButtonSignalUrl, ssid, password, HIGH);
        sendWateringStatus(false);
    }
    else // om mer än wait threshhold, sov o kolla igen om sleep_time tid
    {
        disconnectFromWiFi();
        Serial.printf("Sleeping for %d ms\n", sleep_time);
        delay(sleep_time); // Sleep for the threshold time
        connectToWiFi(ssid, password);
    }
}

void resetLED()
{
    digitalWrite(pinLED, HIGH);
    delay(5000);
    digitalWrite(pinLED, LOW);
    if (digitalRead(pinInput) == HIGH)
    {
        handleLED(0, maxOnDuration, noButtonSignalUrl, ssid, password, HIGH);
    }

    if (digitalRead(pinInput) == LOW)
    {
        handleLED(0, maxOnDuration, noButtonSignalUrl, ssid, password, HIGH);
        if (digitalRead(pinInput) == HIGH)
        {
            handleLED(0, maxOnDuration, noButtonSignalUrl, ssid, password, LOW);
        }
    }
}