#include "WaterValveController.h"
#include "WiFiManager.h"
#include "HTTPHandler.h"
#include "Utils.h"

extern const int pinLED;
extern const int pinInput;
extern const char *set_is_watering_rul;

WaterValveController *WaterValveController::instance = nullptr;

WaterValveController::WaterValveController()
    : gpioInterface(new ArduinoGPIOWrapper()), ownsInterface(true),
      valveControlPin(0), valveSensorPin(0), maxSensorWaitDuration(10000),
      wifiSSID(nullptr), wifiPassword(nullptr),
      wateringStatusUrl(nullptr), noSensorSignalUrl(nullptr)
{
}

WaterValveController::~WaterValveController()
{
    if (ownsInterface && gpioInterface)
    {
        delete gpioInterface;
    }
}

WaterValveController &WaterValveController::getInstance()
{
    if (instance == nullptr)
    {
        instance = new WaterValveController();
    }
    return *instance;
}

void WaterValveController::setGPIOInterface(IGPIOInterface *interface)
{
    if (instance && instance->ownsInterface && instance->gpioInterface)
    {
        delete instance->gpioInterface;
    }
    if (instance)
    {
        instance->gpioInterface = interface;
        instance->ownsInterface = false;
    }
}

void WaterValveController::initialize(uint8_t controlPin, uint8_t sensorPin, unsigned long maxWaitDuration)
{
    valveControlPin = controlPin;
    valveSensorPin = sensorPin;
    maxSensorWaitDuration = maxWaitDuration;

    gpioInterface->pinMode(valveControlPin, OUTPUT);
    gpioInterface->pinMode(valveSensorPin, INPUT_PULLUP);
}

void WaterValveController::setNetworkConfig(const char *ssid, const char *password,
                                            const char *statusUrl, const char *noSensorUrl)
{
    wifiSSID = ssid;
    wifiPassword = password;
    wateringStatusUrl = statusUrl;
    noSensorSignalUrl = noSensorUrl;
}

void WaterValveController::sendWateringStatus(boolean status)
{
    WiFiManager &wifiManager = WiFiManager::getInstance();

    if (wifiManager.isConnected())
    {
        HTTPHandler &httpHandler = HTTPHandler::getInstance();

        // Create JSON payload
        StaticJsonDocument<200> doc;
        doc["isWatering"] = status;
        String payload;
        serializeJson(doc, payload);

        // For now using the existing sendRequest - in real implementation you'd want POST
        String response = httpHandler.sendRequest(wateringStatusUrl ? wateringStatusUrl : set_is_watering_rul);

        Serial.print("Watering status sent: ");
        Serial.println(status ? "true" : "false");
    }
    else
    {
        Serial.println("WiFi Disconnected");
    }
}

void WaterValveController::handleValveSensor(int timeUntilWatering, int maxOnDuration,
                                             const char *noSensorSignalUrl, const char *ssid,
                                             const char *password, int waitState)
{
    // Turn on the valve
    gpioInterface->digitalWrite(valveControlPin, HIGH);
    disconnectWiFi();
    unsigned long startTime = gpioInterface->millis();
    bool sensorSignal = false;

    while (gpioInterface->millis() - startTime < maxOnDuration)
    {
        // Read the sensor state directly without debouncing
        int sensorState = gpioInterface->digitalRead(valveSensorPin);

        // Check if the sensor state matches the wait state
        if (sensorState == waitState)
        {
            sensorSignal = true;
            break;
        }

        gpioInterface->delay(10); // Small delay to prevent high CPU usage
    }

    // Turn off the valve
    gpioInterface->digitalWrite(valveControlPin, LOW);
    connectWiFi();

    // If no sensor signal received, perform shutdown
    if (!sensorSignal)
    {
        HTTPHandler &httpHandler = HTTPHandler::getInstance();
        httpHandler.sendRequest(noSensorSignalUrl);
        shutdown("No sensor signal received, shutting down");
    }
}

void WaterValveController::processResponse(const String &payload)
{
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error)
    {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.f_str());
        return;
    }

    int timeUntilWatering = 1000 * int(doc["time_until_watering"]);
    unsigned long wateringTime = doc["watering_time"] * 1000 * 60; // Access the value as an integer
    int sleepTime = doc["sleep_time"] * 1000;

    Serial.print("time_until_watering after JSON parsing = ");
    Serial.println(timeUntilWatering);
    Serial.print("Valve On Duration = ");
    Serial.println(wateringTime);
    Serial.print("Sleep Time = ");
    Serial.println(sleepTime);

    if (timeUntilWatering < sleepTime + 20000)
    { // 4 timmar + 15 sekunder
        // är här inne om mindre än sleep_time tid tills vattning
        if (timeUntilWatering > 0)
        { // om tiden är mer än 0, alltså vi ska vänta
            disconnectWiFi();
            Serial.printf("Sleeping for %d ms before doing a cycle\n", timeUntilWatering);
            gpioInterface->delay(timeUntilWatering);
            connectWiFi();
        }

        handleValveSensor(timeUntilWatering, maxSensorWaitDuration, noSensorSignalUrl, wifiSSID, wifiPassword, LOW);
        sendWateringStatus(true);
        disconnectWiFi();
        gpioInterface->delay(wateringTime);
        connectWiFi();
        handleValveSensor(timeUntilWatering, maxSensorWaitDuration, noSensorSignalUrl, wifiSSID, wifiPassword, HIGH);
        sendWateringStatus(false);
    }
    else
    { // om mer än wait threshhold, sov o kolla igen om sleep_time tid
        disconnectWiFi();
        Serial.printf("Sleeping for %d ms\n", sleepTime);
        gpioInterface->delay(sleepTime); // Sleep for the threshold time
        connectWiFi();
    }
}

void WaterValveController::resetValve()
{
    gpioInterface->digitalWrite(valveControlPin, HIGH);
    gpioInterface->delay(5000);
    gpioInterface->digitalWrite(valveControlPin, LOW);

    if (gpioInterface->digitalRead(valveSensorPin) == HIGH)
    {
        handleValveSensor(0, maxSensorWaitDuration, noSensorSignalUrl, wifiSSID, wifiPassword, HIGH);
    }

    if (gpioInterface->digitalRead(valveSensorPin) == LOW)
    {
        handleValveSensor(0, maxSensorWaitDuration, noSensorSignalUrl, wifiSSID, wifiPassword, HIGH);
        if (gpioInterface->digitalRead(valveSensorPin) == HIGH)
        {
            handleValveSensor(0, maxSensorWaitDuration, noSensorSignalUrl, wifiSSID, wifiPassword, LOW);
        }
    }
}

void WaterValveController::connectWiFi()
{
    if (wifiSSID && wifiPassword)
    {
        WiFiManager &wifiManager = WiFiManager::getInstance();
        wifiManager.connectToWiFi(wifiSSID, wifiPassword);
    }
}

void WaterValveController::disconnectWiFi()
{
    WiFiManager &wifiManager = WiFiManager::getInstance();
    wifiManager.disconnectFromWiFi();
}

// Keep the old functions for backward compatibility
void resetLED()
{
    WaterValveController::getInstance().resetValve();
}

void processResponse(const String &payload)
{
    WaterValveController::getInstance().processResponse(payload);
}

void handleLED(int timeUntilWatering, int maxOnDuration, const char *noSensorSignalUrl,
               const char *ssid, const char *password, int waitState)
{
    WaterValveController::getInstance().handleValveSensor(timeUntilWatering, maxOnDuration,
                                                          noSensorSignalUrl, ssid, password, waitState);
}

void sendWateringStatus(boolean status)
{
    WaterValveController::getInstance().sendWateringStatus(status);
}