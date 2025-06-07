#pragma once

#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include "ArduinoJson.h"

// Interface for HTTP client operations
class IHTTPClient
{
public:
    virtual ~IHTTPClient() = default;
    virtual bool begin(const String &url) = 0;
    virtual int GET() = 0;
    virtual String getString() = 0;
    virtual void end() = 0;
};

// Interface for WiFi client operations
class IWiFiClient
{
public:
    virtual ~IWiFiClient() = default;
    virtual void setInsecure() = 0; // Only used for secure connections
};

// Wrapper for ESP8266HTTPClient - supports both WiFiClient and WiFiClientSecure
class ESP8266HTTPWrapper : public IHTTPClient
{
private:
    HTTPClient httpClient;
    WiFiClient *regularClient;
    WiFiClientSecure *secureClient;
    bool isSecure;

public:
    // Constructor for regular WiFiClient (HTTP)
    ESP8266HTTPWrapper(WiFiClient *client) : regularClient(client), secureClient(nullptr), isSecure(false) {}

    // Constructor for WiFiClientSecure (HTTPS)
    ESP8266HTTPWrapper(WiFiClientSecure *client) : regularClient(nullptr), secureClient(client), isSecure(true) {}

    bool begin(const String &url) override
    {
        if (isSecure && secureClient)
        {
            return httpClient.begin(*secureClient, url);
        }
        else if (!isSecure && regularClient)
        {
            return httpClient.begin(*regularClient, url);
        }
        return false;
    }

    int GET() override
    {
        return httpClient.GET();
    }

    String getString() override
    {
        return httpClient.getString();
    }

    void end() override
    {
        httpClient.end();
    }
};

// Wrapper for WiFiClient - supports both types
class ESP8266WiFiClientWrapper : public IWiFiClient
{
private:
    WiFiClient *regularClient;
    WiFiClientSecure *secureClient;
    bool isSecure;

public:
    // Constructor for regular WiFiClient (HTTP)
    ESP8266WiFiClientWrapper(WiFiClient *client) : regularClient(client), secureClient(nullptr), isSecure(false) {}

    // Constructor for WiFiClientSecure (HTTPS)
    ESP8266WiFiClientWrapper(WiFiClientSecure *client) : regularClient(nullptr), secureClient(client), isSecure(true) {}

    void setInsecure() override
    {
        // Only call setInsecure on WiFiClientSecure
        if (isSecure && secureClient)
        {
            secureClient->setInsecure();
        }
        // For regular WiFiClient, this is a no-op since it's already "insecure"
    }
};

enum class HTTPStatus
{
    SUCCESS,
    CONNECTION_FAILED,
    REQUEST_FAILED,
    INVALID_RESPONSE
};

class HTTPHandler
{
private:
    static HTTPHandler *instance;
    IHTTPClient *httpClient;
    IWiFiClient *wifiClient;
    bool ownsClients;
    HTTPStatus lastStatus;

    HTTPHandler();

public:
    ~HTTPHandler();
    static HTTPHandler &getInstance();
    static void setClients(IHTTPClient *http, IWiFiClient *wifi);

    String sendRequest(const String &url);
    HTTPStatus getLastStatus() const;

    // Specific methods for different endpoints
    String getDeviceVariables(const String &serverUrl);
    bool sendNoButtonSignal(const String &url);
    bool setIsWatering(const String &url, bool isWatering);
};

// Keep the old function for backward compatibility
String sendRequestToServer(const char *serverUrl);