#include <iostream>
#include <cassert>
#include <string>

// Mock Arduino functions for testing on dev machine
void delay(unsigned long /*ms*/)
{
    // Simulate delay - parameter used implicitly for timing simulation
}

unsigned long millis()
{
    static unsigned long mockTime = 0;
    mockTime += 100; // Simulate time passing
    return mockTime;
}

class Serial_Mock
{
public:
    template <typename T>
    void print(T value) { std::cout << value; }

    template <typename T>
    void println(T value) { std::cout << value << std::endl; }

    void println() { std::cout << std::endl; }
} Serial;

// Include mock interface (note the path from root directory)
#include "mock/MockWiFiInterface.h"

// Mock the WiFiManager enum and classes for testing
enum class WiFiStatus
{
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    FAILED
};

// Include a simplified version of WiFiManager for testing
class WiFiManager
{
private:
    static const unsigned long RECONNECT_TIMEOUT = 5000;        // Shorter for tests
    static const unsigned long CONNECTION_CHECK_INTERVAL = 100; // Faster for tests

    static WiFiManager *instance;
    IWiFiInterface *wifiInterface;
    WiFiStatus currentStatus;
    unsigned long connectionStartTime;
    bool ownsInterface;

    WiFiManager() : wifiInterface(nullptr), currentStatus(WiFiStatus::DISCONNECTED),
                    connectionStartTime(0), ownsInterface(false) {} // Don't create default interface

public:
    static WiFiManager &getInstance()
    {
        if (instance == nullptr)
        {
            instance = new WiFiManager();
        }
        return *instance;
    }

    static void setWiFiInterface(IWiFiInterface *interface)
    {
        // Ensure we have an instance first
        WiFiManager &mgr = getInstance();

        if (mgr.ownsInterface && mgr.wifiInterface)
        {
            delete mgr.wifiInterface;
        }

        mgr.wifiInterface = interface;
        mgr.ownsInterface = false; // We don't own the test interface
    }

    void initializePowerSaving()
    {
        std::cout << "Initializing power saving WiFi settings..." << std::endl;
        // Mock implementation
    }

    bool connectToWiFi(const char *ssid, const char *password)
    {
        if (!wifiInterface)
        {
            std::cout << "No WiFi interface set!" << std::endl;
            return false;
        }

        currentStatus = WiFiStatus::CONNECTING;

        std::cout << "Connecting to " << ssid << std::endl;

        wifiInterface->begin(ssid, password);
        connectionStartTime = millis();

        // Check connection status after begin() call
        if (wifiInterface->status() == WL_CONNECTED)
        {
            currentStatus = WiFiStatus::CONNECTED;
            std::cout << "\nWiFi connected" << std::endl;
            std::cout << "IP address: " << getLocalIP().c_str() << std::endl;
            return true;
        }
        else
        {
            // Simulate timeout attempts for failed connections
            int attempts = 0;
            while (wifiInterface->status() != WL_CONNECTED && attempts < 5)
            {
                delay(CONNECTION_CHECK_INTERVAL);
                std::cout << ".";
                attempts++;
            }

            if (wifiInterface->status() != WL_CONNECTED)
            {
                std::cout << "\nFailed to connect." << std::endl;
                currentStatus = WiFiStatus::FAILED;
                return false;
            }
            else
            {
                currentStatus = WiFiStatus::CONNECTED;
                std::cout << "\nWiFi connected" << std::endl;
                std::cout << "IP address: " << getLocalIP().c_str() << std::endl;
                return true;
            }
        }
    }

    void disconnectFromWiFi()
    {
        if (!wifiInterface)
            return;

        std::cout << "Disconnecting WiFi" << std::endl;
        wifiInterface->mode(WIFI_OFF);
        currentStatus = WiFiStatus::DISCONNECTED;
    }

    bool isConnected() const
    {
        return currentStatus == WiFiStatus::CONNECTED;
    }

    WiFiStatus getStatus() const
    {
        return currentStatus;
    }

    String getLocalIP() const
    {
        if (currentStatus == WiFiStatus::CONNECTED && wifiInterface)
        {
            return wifiInterface->localIP();
        }
        return "0.0.0.0";
    }

    // Reset for testing
    static void reset()
    {
        if (instance)
        {
            if (instance->ownsInterface && instance->wifiInterface)
            {
                delete instance->wifiInterface;
            }
            delete instance;
            instance = nullptr;
        }
    }
};

WiFiManager *WiFiManager::instance = nullptr;

// Test functions
void test_wifi_connection_success()
{
    std::cout << "=== Testing successful WiFi connection ===" << std::endl;

    WiFiManager::reset(); // Clean state for test

    MockWiFiInterface *mockWiFi = new MockWiFiInterface();
    mockWiFi->setConnectionSuccess(true);

    WiFiManager::setWiFiInterface(mockWiFi);
    WiFiManager &wifiManager = WiFiManager::getInstance();

    bool connected = wifiManager.connectToWiFi("TestSSID", "TestPassword");

    assert(connected == true);
    assert(wifiManager.isConnected() == true);
    assert(wifiManager.getStatus() == WiFiStatus::CONNECTED);
    assert(wifiManager.getLocalIP() == "192.168.1.100");

    std::cout << "✓ Connection success test passed" << std::endl;

    delete mockWiFi; // Clean up
}

void test_wifi_connection_failure()
{
    std::cout << "=== Testing WiFi connection failure ===" << std::endl;

    WiFiManager::reset(); // Clean state for test

    MockWiFiInterface *mockWiFi = new MockWiFiInterface();
    mockWiFi->setConnectionSuccess(false); // This should make connection fail

    WiFiManager::setWiFiInterface(mockWiFi);
    WiFiManager &wifiManager = WiFiManager::getInstance();

    bool connected = wifiManager.connectToWiFi("TestSSID", "WrongPassword");

    assert(connected == false);
    assert(wifiManager.isConnected() == false);
    assert(wifiManager.getStatus() == WiFiStatus::FAILED);
    assert(wifiManager.getLocalIP() == "0.0.0.0");

    std::cout << "✓ Connection failure test passed" << std::endl;

    delete mockWiFi; // Clean up
}

void test_wifi_disconnect()
{
    std::cout << "=== Testing WiFi disconnect ===" << std::endl;

    WiFiManager::reset(); // Clean state for test

    MockWiFiInterface *mockWiFi = new MockWiFiInterface();
    mockWiFi->setConnectionSuccess(true);

    WiFiManager::setWiFiInterface(mockWiFi);
    WiFiManager &wifiManager = WiFiManager::getInstance();

    wifiManager.connectToWiFi("TestSSID", "TestPassword");
    assert(wifiManager.isConnected() == true);

    wifiManager.disconnectFromWiFi();
    assert(wifiManager.isConnected() == false);
    assert(wifiManager.getStatus() == WiFiStatus::DISCONNECTED);

    std::cout << "✓ Disconnect test passed" << std::endl;

    delete mockWiFi; // Clean up
}

int main()
{
    std::cout << "Running WiFiManager Tests..." << std::endl
              << std::endl;

    try
    {
        test_wifi_connection_success();
        test_wifi_connection_failure();
        test_wifi_disconnect();

        std::cout << std::endl
                  << "🎉 All tests passed!" << std::endl;
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cout << "❌ Test failed: " << e.what() << std::endl;
        return 1;
    }
}