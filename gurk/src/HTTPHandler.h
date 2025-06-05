#ifndef HTTPHANDLER_H
#define HTTPHANDLER_H

#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Arduino.h>

// Abstract interface for HTTP operations (for mocking)
class IHTTPClient
{
public:
    virtual ~IHTTPClient() = default;
    virtual bool begin(const String &url) = 0;
    virtual int GET() = 0;
    virtual String getString() = 0;
    virtual void end() = 0;
};

// Abstract interface for WiFi Client operations (for mocking)
class IWiFiClient
{
public:
    virtual ~IWiFiClient() = default;
    virtual void setInsecure() = 0;
};

// Real HTTP implementation
class ESP8266HTTPWrapper : public IHTTPClient
{
private:
    HTTPClient *httpClient;
    WiFiClientSecure *wifiClient;

public:
    ESP8266HTTPWrapper(WiFiClientSecure *client) : wifiClient(client)
    {
        httpClient = new HTTPClient();
    }

    ~ESP8266HTTPWrapper()
    {
        delete httpClient;
    }

    bool begin(const String &url) override
    {
        return httpClient->begin(*wifiClient, url);
    }

    int GET() override
    {
        return httpClient->GET();
    }

    String getString() override
    {
        return httpClient->getString();
    }

    void end() override
    {
        httpClient->end();
    }
};

// Real WiFi Client implementation
class ESP8266WiFiClientWrapper : public IWiFiClient
{
private:
    WiFiClientSecure *client;

public:
    ESP8266WiFiClientWrapper(WiFiClientSecure *wifiClient) : client(wifiClient) {}

    void setInsecure() override
    {
        client->setInsecure();
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

    HTTPHandler();

public:
    static HTTPHandler &getInstance();
    static void setClients(IHTTPClient *httpClient, IWiFiClient *wifiClient); // For testing

    ~HTTPHandler();

    String sendRequest(const String &url);
    HTTPStatus getLastStatus() const;

    // Specific methods for your endpoints
    String getDeviceVariables(const String &serverUrl);
    bool sendNoButtonSignal(const String &url);
    bool setIsWatering(const String &url, bool isWatering);

private:
    HTTPStatus lastStatus;
};

#endif