#ifndef WIFIMANAGER_H
#define WIFIMANAGER_H

#include <ESP8266WiFi.h>

enum class WiFiStatus
{
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    FAILED
};

// Abstract interface for WiFi operations (for mocking)
class IWiFiInterface
{
public:
    virtual ~IWiFiInterface() = default;
    virtual void begin(const char *ssid, const char *password) = 0;
    virtual int status() = 0;
    virtual void disconnect() = 0;
    virtual void mode(WiFiMode_t mode) = 0;
    virtual bool forceSleepBegin() = 0;
    virtual bool forceSleepWake() = 0;
    virtual void persistent(bool persistent) = 0;
    virtual String localIP() = 0;
};

// Real WiFi implementation
class ESP8266WiFiWrapper : public IWiFiInterface
{
public:
    void begin(const char *ssid, const char *password) override { WiFi.begin(ssid, password); }
    int status() override { return WiFi.status(); }
    void disconnect() override { WiFi.disconnect(); }
    void mode(WiFiMode_t mode) override { WiFi.mode(mode); }
    bool forceSleepBegin() override { return WiFi.forceSleepBegin(); }
    bool forceSleepWake() override { return WiFi.forceSleepWake(); }
    void persistent(bool persistent) override { WiFi.persistent(persistent); }
    String localIP() override { return WiFi.localIP().toString(); }
};

class WiFiManager
{
private:
    static const unsigned long RECONNECT_TIMEOUT = 120000;      // 2 minutes
    static const unsigned long STANDBY_DURATION = 7200000;      // 2 hours
    static const unsigned long CONNECTION_CHECK_INTERVAL = 500; // 0.5 seconds

    static WiFiManager *instance;
    IWiFiInterface *wifiInterface;
    WiFiStatus currentStatus;
    unsigned long connectionStartTime;
    bool ownsInterface;

    WiFiManager();

public:
    static WiFiManager &getInstance();
    static void setWiFiInterface(IWiFiInterface *interface); // For testing

    ~WiFiManager();

    bool connectToWiFi(const char *ssid, const char *password);
    void disconnectFromWiFi();
    bool isConnected() const;
    WiFiStatus getStatus() const;
    String getLocalIP() const;

    // Power saving setup (moved from main.cpp)
    void initializePowerSaving();
};

#endif