#include "WiFiManager.h"

const unsigned long reconnectInterval = 5000;  // Interval for attempting to reconnect (in milliseconds)
const unsigned long reconnectTimeout = 60000;  // Maximum time to attempt reconnection (in milliseconds)
const unsigned long standbyDuration = 7200000; // Duration for standby mode (in milliseconds, 2 hours)

void standbyMode(unsigned long standbyDuration, unsigned long reconnectTimeout)
{

    delay(standbyDuration); // Wait for the standby duration

    // Attempt to reconnect after standby duration
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

    // If reconnection attempt fails, enter standby mode again
    Serial.println("Failed to reconnect after standby. Entering standby mode again.");
    standbyMode(standbyDuration, reconnectTimeout);
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
            standbyMode(standbyDuration, reconnectTimeout); // Assume LED pin is 4 for this context
            startTime = millis();
        }
    }
    Serial.println("\nReconnected to WiFi!");
}

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
        if (millis() - startTime > reconnectTimeout)
        {
            Serial.println("\nFailed to connect to WiFi within the timeout period. Entering standby mode.");
            go_to_sleep(standbyDuration);
        }
    }

    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void disconnectFromWiFi()
{
    Serial.println("Disconnecting WiFi");
    WiFi.mode(WIFI_OFF);
}
