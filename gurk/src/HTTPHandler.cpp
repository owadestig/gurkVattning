#include "HTTPHandler.h"

HTTPHandler *HTTPHandler::instance = nullptr;

HTTPHandler::HTTPHandler() : httpClient(nullptr), wifiClient(nullptr), ownsClients(true), lastStatus(HTTPStatus::SUCCESS)
{
    WiFiClient *realWifiClient = new WiFiClient();
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
        Serial.println("HTTP Debug: Client not initialized");
        lastStatus = HTTPStatus::CONNECTION_FAILED;
        return "error";
    }

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
            Serial.println(payload);
        }
        else
        {
            Serial.println(httpCode);
            lastStatus = HTTPStatus::INVALID_RESPONSE;
        }
    }
    else
    {
        Serial.println(httpCode);
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
    String response = sendRequest(url);
    return response != "error" && lastStatus == HTTPStatus::SUCCESS;
}

String sendRequestToServer(const char *serverUrl)
{
    return HTTPHandler::getInstance().sendRequest(String(serverUrl));
}