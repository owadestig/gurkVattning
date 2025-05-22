#include "HTTPHandler.h"

HTTPHandler *HTTPHandler::instance = nullptr;

HTTPHandler::HTTPHandler() : httpClient(nullptr), wifiClient(nullptr), ownsClients(true), lastStatus(HTTPStatus::SUCCESS)
{
    WiFiClientSecure *realWifiClient = new WiFiClientSecure();
    wifiClient = new ESP8266WiFiClientWrapper(realWifiClient);
    httpClient = new ESP8266HTTPWrapper(realWifiClient);
}

HTTPHandler::~HTTPHandler()
{
    if (ownsClients)
    {
        delete httpClient;
        delete wifiClient;
    }
}

HTTPHandler &HTTPHandler::getInstance()
{
    if (instance == nullptr)
    {
        instance = new HTTPHandler();
    }
    return *instance;
}

void HTTPHandler::setClients(IHTTPClient *http, IWiFiClient *wifi)
{
    if (instance && instance->ownsClients)
    {
        delete instance->httpClient;
        delete instance->wifiClient;
    }
    if (instance)
    {
        instance->httpClient = http;
        instance->wifiClient = wifi;
        instance->ownsClients = false;
    }
}

String HTTPHandler::sendRequest(const String &url)
{
    if (!httpClient || !wifiClient)
    {
        lastStatus = HTTPStatus::CONNECTION_FAILED;
        return "error";
    }

    wifiClient->setInsecure(); // Disable SSL certificate verification

    if (!httpClient->begin(url))
    {
        lastStatus = HTTPStatus::CONNECTION_FAILED;
        return "error";
    }

    int httpCode = httpClient->GET();
    String payload = "error";

    if (httpCode > 0)
    {
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
        {
            payload = httpClient->getString();
            lastStatus = HTTPStatus::SUCCESS;
        }
        else
        {
            lastStatus = HTTPStatus::INVALID_RESPONSE;
        }
    }
    else
    {
        lastStatus = HTTPStatus::REQUEST_FAILED;
    }

    httpClient->end();
    return payload;
}

HTTPStatus HTTPHandler::getLastStatus() const
{
    return lastStatus;
}

String HTTPHandler::getDeviceVariables(const String &serverUrl)
{
    return sendRequest(serverUrl);
}

bool HTTPHandler::sendNoButtonSignal(const String &url)
{
    String response = sendRequest(url);
    return response != "error" && lastStatus == HTTPStatus::SUCCESS;
}

bool HTTPHandler::setIsWatering(const String &url, bool isWatering)
{
    // You might want to modify this to send POST data with the isWatering parameter
    String response = sendRequest(url);
    return response != "error" && lastStatus == HTTPStatus::SUCCESS;
}

// Keep the old function for backward compatibility
String sendRequestToServer(const char *serverUrl)
{
    return HTTPHandler::getInstance().sendRequest(String(serverUrl));
}