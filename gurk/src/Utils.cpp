#include "Utils.h"
#include "WifiManager.h"

void shutdown(String message)
{
    Serial.println(message);
    // TODO: IMPLEMENT TURN OFF CODE HERE
}

void go_to_sleep(int sleepTime)
{
    WiFi.disconnect(true);
    delay(1);

    // WAKE_RF_DISABLED to keep the WiFi radio disabled when we wake up
    ESP.deepSleep(sleepTime, WAKE_RF_DISABLED);
}