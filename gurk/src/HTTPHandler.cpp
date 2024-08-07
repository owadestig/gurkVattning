#include "HTTPHandler.h"

String sendRequestToServer(const char *serverUrl)
{
    WiFiClientSecure client;
    HTTPClient https;
    String payload = "error";
    client.setInsecure(); // Disable SSL certificate verification

    if (https.begin(client, serverUrl))
    {
        int httpCode = https.GET();
        if (httpCode > 0)
        {
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
            {
                String payload = https.getString();
            }
        }
        https.end();
    }
    return payload;
}