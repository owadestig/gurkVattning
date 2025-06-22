#include "WaterController.h"
#include "WiFiManager.h"
#include "HTTPHandler.h"

extern const int pinLED;
extern const int pinInput;
extern const char *set_is_watering_rul;

WaterController *WaterController::instance = nullptr;

WaterController::WaterController()
    : gpioInterface(new ArduinoGPIOWrapper()), ownsInterface(true),
      valveControlPin(0), valveSensorPin(0), maxSensorWaitDuration(10000),
      wifiSSID(nullptr), wifiPassword(nullptr),
      wateringStatusUrl(nullptr), noSensorSignalUrl(nullptr)
{
}

WaterController::~WaterController()
{
    if (ownsInterface && gpioInterface)
    {
        delete gpioInterface;
    }
}

WaterController &WaterController::getInstance()
{
    if (instance == nullptr)
    {
        instance = new WaterController();
    }
    return *instance;
}

void WaterController::setGPIOInterface(IGPIOInterface *interface)
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

void WaterController::initialize(uint8_t controlPin, uint8_t sensorPin, unsigned long maxWaitDuration)
{
    valveControlPin = controlPin;
    valveSensorPin = sensorPin;
    maxSensorWaitDuration = maxWaitDuration;

    gpioInterface->pinMode(valveControlPin, OUTPUT);
    gpioInterface->pinMode(valveSensorPin, INPUT_PULLUP);
}

void WaterController::setNetworkConfig(const char *ssid, const char *password,
                                       const char *statusUrl, const char *noSensorUrl)
{
    wifiSSID = ssid;
    wifiPassword = password;
    wateringStatusUrl = statusUrl;
    noSensorSignalUrl = noSensorUrl;
}

void WaterController::sendWateringStatus(boolean status)
{
    WiFiManager &wifiManager = WiFiManager::getInstance();

    if (wifiManager.isConnected())
    {
        HTTPHandler &httpHandler = HTTPHandler::getInstance();

        StaticJsonDocument<512> doc;
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

void WaterController::handleValveSensor(int maxOnDuration,
                                        const char *noSensorSignalUrl, const char *ssid,
                                        const char *password, int waitState)
{
    // Turn on the valve
    gpioInterface->digitalWrite(valveControlPin, HIGH);
    unsigned long startTime = gpioInterface->millis();
    bool sensorSignal = false;

    // Fix: Cast maxOnDuration to unsigned long to avoid signed/unsigned comparison warning
    while (gpioInterface->millis() - startTime < (unsigned long)maxOnDuration)
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

    // Message if no signal is received
    if (!sensorSignal)
    {
        HTTPHandler &httpHandler = HTTPHandler::getInstance();
        httpHandler.sendRequest(noSensorSignalUrl);
    }
}

void WaterController::processResponse(const String &payload)
{
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error)
    {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.f_str());
        return;
    }

    int timeUntilWatering = 1000 * int(doc["time_until_watering"]);
    unsigned long valveOnDuration = doc["valve_on_duration_minutes"].as<int>() * 1000UL * 60UL;
    int sleepTime = doc["sleep_time"].as<int>() * 1000;

    Serial.print("time_until_watering after JSON parsing = ");
    Serial.println(timeUntilWatering);
    Serial.print("Valve On Duration = ");
    Serial.println(valveOnDuration);
    Serial.print("Sleep Time = ");
    Serial.println(sleepTime);

    if (timeUntilWatering < sleepTime + 20000)
    { // 4 timmar + 15 sekunder
        // är här inne om mindre än sleep_time tid tills vattning
        if (timeUntilWatering > 0)
        { // om tiden är mer än 0, alltså vi ska vänta
            Serial.printf("Sleeping for %d ms before doing a cycle\n", timeUntilWatering);
            gpioInterface->delay(timeUntilWatering);
        }
        waterLoop(valveOnDuration,
                  maxSensorWaitDuration, noSensorSignalUrl,
                  wifiSSID, wifiPassword);
    }
    else
    { // om mer än wait threshhold, sov o kolla igen om sleep_time tid
        Serial.printf("Sleeping for %d ms\n", sleepTime);
        gpioInterface->delay(sleepTime); // Sleep for the threshold time
    }
}

void WaterController::waterLoop(unsigned long valveOnDuration,
                                int maxSensorWaitDuration, const char *noSensorSignalUrl,
                                const char *wifiSSID, const char *wifiPassword)
{
    handleValveSensor(maxSensorWaitDuration, noSensorSignalUrl, wifiSSID, wifiPassword, LOW);
    sendWateringStatus(true);
    gpioInterface->delay(valveOnDuration);
    handleValveSensor(maxSensorWaitDuration, noSensorSignalUrl, wifiSSID, wifiPassword, HIGH);
    sendWateringStatus(false);
}

void WaterController::resetValve()
{
    gpioInterface->digitalWrite(valveControlPin, HIGH);
    gpioInterface->delay(5000);
    gpioInterface->digitalWrite(valveControlPin, LOW);

    if (gpioInterface->digitalRead(valveSensorPin) == HIGH)
    {
        handleValveSensor(maxSensorWaitDuration, noSensorSignalUrl, wifiSSID, wifiPassword, HIGH);
    }

    if (gpioInterface->digitalRead(valveSensorPin) == LOW)
    {
        handleValveSensor(maxSensorWaitDuration, noSensorSignalUrl, wifiSSID, wifiPassword, HIGH);
        if (gpioInterface->digitalRead(valveSensorPin) == HIGH)
        {
            handleValveSensor(maxSensorWaitDuration, noSensorSignalUrl, wifiSSID, wifiPassword, LOW);
        }
    }
}

void WaterController::connectWiFi()
{
    if (wifiSSID && wifiPassword)
    {
        WiFiManager &wifiManager = WiFiManager::getInstance();
        wifiManager.connectToWiFi(wifiSSID, wifiPassword);
    }
}

void WaterController::disconnectWiFi()
{
    WiFiManager &wifiManager = WiFiManager::getInstance();
    wifiManager.disconnectFromWiFi();
}

void processResponse(const String &payload)
{
    WaterController::getInstance().processResponse(payload);
}

void sendWateringStatus(boolean status)
{
    WaterController::getInstance().sendWateringStatus(status);
}

void WaterController::reset()
{
    if (instance)
    {
        delete instance;
        instance = nullptr;
    }
}