#include "Utils.h"

void shutdown(String message)
{
    Serial.println(message);
}

void go_to_sleep(int sleepTime)
{
    Serial.printf("sleeptime in here is = %d\n", sleepTime);
    WiFi.disconnect(true);
    delay(sleepTime);

    // WAKE_RF_DISABLED to keep the WiFi radio disabled when we wake up
    // ESP.deepSleep(sleepTime * 1000, RF_DEFAULT);
    ESP.reset(); // Reset and try again
}