#include "Utils.h"

void shutdown(String message)
{
    Serial.println(message);
}

void go_to_sleep(int sleepTime)
{
    Serial.printf("Going to sleep for %d ms\n", sleepTime);
    delay(sleepTime); // Simulate sleep
    Serial.println("Waking up from sleep");
}