#ifndef WATERVALVECONTROLLER_H
#define WATERVALVECONTROLLER_H

#ifdef TEST_ENVIRONMENT
#include "../test/mock/ArduinoMock.h"
#include "../.pio/libdeps/modwifi/ArduinoJson/src/ArduinoJson.h"
#else
#include <Arduino.h>
#include <ArduinoJson.h>
#endif

// Forward declarations
class WiFiManager;
class HTTPHandler;

// Abstract interface for GPIO operations (for mocking)
class IGPIOInterface
{
public:
    virtual ~IGPIOInterface() = default;
    virtual void pinMode(uint8_t pin, uint8_t mode) = 0;
    virtual void digitalWrite(uint8_t pin, uint8_t value) = 0;
    virtual int digitalRead(uint8_t pin) = 0;
    virtual void delay(unsigned long ms) = 0;
    virtual unsigned long millis() = 0;
};
class ArduinoGPIOWrapper : public IGPIOInterface
{
public:
    void pinMode(uint8_t pin, uint8_t mode) override
    {
        ::pinMode(pin, mode);
    }

    void digitalWrite(uint8_t pin, uint8_t value) override
    {
        ::digitalWrite(pin, value);
    }

    int digitalRead(uint8_t pin) override
    {
        return ::digitalRead(pin);
    }

    void delay(unsigned long ms) override
    {
        ::delay(ms);
    }

    unsigned long millis() override
    {
        return ::millis();
    }
};

struct WateringSchedule
{
    int timeUntilWatering;         // milliseconds until watering should start
    unsigned long valveOnDuration; // how long to water (milliseconds)
    int sleepTime;                 // how long to sleep between checks (milliseconds)
};

class WaterController
{
private:
    static WaterController *instance;
    IGPIOInterface *gpioInterface;
    bool ownsInterface;

    // Pin configurations
    uint8_t valveControlPin;             // Controls valve
    uint8_t valveSensorPin;              // Senses valve position
    unsigned long maxSensorWaitDuration; // How long to wait for sensor confirmation

    WaterController();

public:
    static WaterController &getInstance();
    static void setGPIOInterface(IGPIOInterface *interface);
    static void reset();

    ~WaterController();

    void initialize(uint8_t controlPin, uint8_t sensorPin, unsigned long maxWaitDuration);
    void setNetworkConfig(const char *ssid, const char *password,
                          const char *statusUrl, const char *noSensorSignalUrl);

    void processResponse(const String &payload);
    void resetValve();
    void handleValveSensor(int maxOnDuration,
                           const char *noSensorSignalUrl, const char *ssid,
                           const char *password, int waitState);

    void WaterController::waterLoop(unsigned long valveOnDuration,
                                    int maxSensorWaitDuration, const char *noSensorSignalUrl,
                                    const char *wifiSSID, const char *wifiPassword);
    // Network operations
    void sendWateringStatus(boolean status);

private:
    // Network configuration
    const char *wifiSSID;
    const char *wifiPassword;
    const char *wateringStatusUrl;
    const char *noSensorSignalUrl;

    void connectWiFi();
    void disconnectWiFi();
};

#endif