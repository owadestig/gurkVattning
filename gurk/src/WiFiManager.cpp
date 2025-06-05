#include "WiFiManager.h"
#include "Utils.h"
#include <Arduino.h>

WiFiManager *WiFiManager::instance = nullptr;

WiFiManager::WiFiManager()
    : wifiInterface(new ESP8266WiFiWrapper()), currentStatus(WiFiStatus::DISCONNECTED),
      connectionStartTime(0), ownsInterface(true)
{
}

WiFiManager::~WiFiManager()
{
    if (ownsInterface && wifiInterface)
    {
        delete wifiInterface;
    }
}

WiFiManager &WiFiManager::getInstance()
{
    if (instance == nullptr)
    {
        instance = new WiFiManager();
    }
    return *instance;
}

void WiFiManager::setWiFiInterface(IWiFiInterface *interface)
{
    if (instance && instance->ownsInterface && instance->wifiInterface)
    {
        delete instance->wifiInterface;
    }
    if (instance)
    {
        instance->wifiInterface = interface;
        instance->ownsInterface = false;
    }
}

void WiFiManager::initializePowerSaving()
{
    Serial.println("Initializing power saving WiFi settings...");

    // Power saving setup from main.cpp
    wifiInterface->mode(WIFI_OFF);
    wifiInterface->forceSleepBegin();
    delay(1);
    wifiInterface->forceSleepWake();
    delay(1);

    // Disable WiFi persistence
    wifiInterface->persistent(false);
    wifiInterface->mode(WIFI_STA);
}

bool WiFiManager::connectToWiFi(const char *ssid, const char *password)
{
    currentStatus = WiFiStatus::CONNECTING;

    Serial.print("Connecting to ");
    Serial.println(ssid);

    wifiInterface->begin(ssid, password);
    connectionStartTime = millis();

    while (wifiInterface->status() != WL_CONNECTED)
    {
        delay(CONNECTION_CHECK_INTERVAL);
        Serial.print(".");

        if (millis() - connectionStartTime > RECONNECT_TIMEOUT)
        {
            Serial.println("\nFailed to connect. Entering standby mode.");
            currentStatus = WiFiStatus::FAILED;
            delay(STANDBY_DURATION);
            return false;
        }
    }

    currentStatus = WiFiStatus::CONNECTED;
    Serial.println("\nWiFi connected");
    Serial.println("IP address: ");
    Serial.println(getLocalIP());

    return true;
}

void WiFiManager::disconnectFromWiFi()
{
    Serial.println("Disconnecting WiFi");
    wifiInterface->mode(WIFI_OFF);
    currentStatus = WiFiStatus::DISCONNECTED;
}

bool WiFiManager::isConnected() const
{
    return currentStatus == WiFiStatus::CONNECTED;
}

WiFiStatus WiFiManager::getStatus() const
{
    return currentStatus;
}

String WiFiManager::getLocalIP() const
{
    if (currentStatus == WiFiStatus::CONNECTED)
    {
        return wifiInterface->localIP();
    }
    return "0.0.0.0";
}