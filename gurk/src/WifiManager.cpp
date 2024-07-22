#include <WiFiManager.h>

const unsigned long RECONNECT_INTERVAL = 5000;  // 5 seconds
const unsigned long RECONNECT_TIMEOUT = 120000; // 2 minutes
const unsigned long STANDBY_DURATION = 7200000; // 2 hours

void connectToWiFi(const char *ssid, const char *password)
{
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);

    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
        if (millis() - startTime > RECONNECT_TIMEOUT)
        {
            Serial.println("\nFailed to connect. Entering standby mode.");
            delay(STANDBY_DURATION);
            return;
        }
    }

    Serial.println("\nWiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void reconnectWiFi()
{
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("Attempting to reconnect to WiFi...");
        WiFi.reconnect();

        unsigned long startTime = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startTime < RECONNECT_TIMEOUT)
        {
            delay(RECONNECT_INTERVAL);
            Serial.print(".");
        }

        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.println("\nReconnected to WiFi!");
        }
        else
        {
            Serial.println("\nFailed to reconnect. Entering standby mode.");
            delay(STANDBY_DURATION);
        }
    }
}

void disconnectFromWiFi()
{
    Serial.println("Disconnecting WiFi");
    WiFi.mode(WIFI_OFF);
}
