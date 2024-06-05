#include "HTTPHandler.h"

String sendHttpRequest(const char *url)
{
    if (WiFi.status() == WL_CONNECTED)
    {
        WiFiClient client;
        HTTPClient http;
        http.begin(client, url);
        int httpCode = http.GET();
        if (httpCode > 0)
        {
            String payload = http.getString();
            Serial.println(httpCode);
            Serial.println(payload);
            http.end();
            return payload;
        }
        else
        {
            Serial.print("Error on HTTP request: ");
            Serial.println(http.errorToString(httpCode).c_str());
            http.end();
            return "";
        }
    }
    else
    {
        Serial.println("WiFi Disconnected");
        reconnectWiFi();
        return "";
    }
}

void sendRequestToServer(const char *url)
{
    if (WiFi.status() == WL_CONNECTED)
    {
        WiFiClient client;
        HTTPClient http;
        http.begin(client, url);
        int httpCode = http.GET();
        if (httpCode > 0)
        {
            String payload = http.getString();
            Serial.println(httpCode);
            Serial.println(payload);
        }
        else
        {
            Serial.print("Error on HTTP request: ");
            Serial.println(http.errorToString(httpCode).c_str());
        }
        http.end();
    }
    else
    {
        Serial.println("WiFi Disconnected");
        reconnectWiFi();
    }
}

int check_if_server_is_up(const char *url)
{
    if (!Ping.ping(url))
    {
        Serial.println("Server is down, doing basic script...");
        return 0;
    }
    return 1;
}