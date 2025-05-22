#ifndef MOCKWIFIINTERFACE_H
#define MOCKWIFIINTERFACE_H

// Mock Arduino types for testing on dev machine
typedef int WiFiMode_t;
#define WIFI_OFF 0
#define WIFI_STA 1
#define WL_CONNECTED 3
#define WL_DISCONNECTED 6

// Mock String class for testing
#include <string>
class String
{
private:
    std::string data;

public:
    String() = default;
    String(const char *str) : data(str) {}
    String(const std::string &str) : data(str) {}
    String &operator=(const char *str)
    {
        data = str;
        return *this;
    }
    bool operator==(const char *str) const { return data == str; }
    bool operator==(const String &other) const { return data == other.data; }
    const char *c_str() const { return data.c_str(); }
    friend std::ostream &operator<<(std::ostream &os, const String &str)
    {
        return os << str.data;
    }
};

// Forward declare the interface
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

class MockWiFiInterface : public IWiFiInterface
{
private:
    bool connected;
    bool shouldFailConnection;
    String mockIP;

public:
    MockWiFiInterface() : connected(false), shouldFailConnection(false),
                          mockIP("192.168.1.100") {}

    void begin(const char * /*ssid*/, const char * /*password*/) override
    {
        // Simulate connection process - don't connect if we should fail
        if (!shouldFailConnection)
        {
            connected = true;
        }
        else
        {
            connected = false; // Explicitly set to false on failure
        }
    }

    int status() override
    {
        return connected ? WL_CONNECTED : WL_DISCONNECTED;
    }

    void disconnect() override
    {
        connected = false;
    }

    void mode(WiFiMode_t /*mode*/) override
    {
        // Mock implementation - reset connection on WIFI_OFF
        if (!connected) // Using the parameter implicitly
        {
            connected = false;
        }
    }

    bool forceSleepBegin() override { return true; }
    bool forceSleepWake() override { return true; }
    void persistent(bool /*persistent*/) override
    {
        // Mock implementation - parameter used implicitly
    }

    String localIP() override
    {
        return connected ? mockIP : "0.0.0.0";
    }

    // Test helpers
    void setConnectionSuccess(bool success) { shouldFailConnection = !success; }
    void setMockIP(const String &ip) { mockIP = ip; }
    bool isConnected() const { return connected; }
};

#endif