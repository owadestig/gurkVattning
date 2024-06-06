#include "HTTPHandler.h"

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
    bool p = Ping.ping(url);
    if (p == true)
    {
        Serial.print("ITS TRUE");
    }
    else
    {
        Serial.print("ITS FALSE");
    }

    if (!Ping.ping(url))
    {
        Serial1.print("WHAT DA FUCK VI ÄR HÄR");
        Serial.println("Server is down, doing basic script...");
        return 0;
    }
    return 1;
}