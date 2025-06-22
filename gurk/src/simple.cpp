#include <Arduino.h>

const int WATER_PIN = 16;
const int SENSOR_PIN = 2;

void waterLoop(unsigned long valveOnDuration,
               int maxSensorWaitDuration)
{
    handleValveSensor(maxSensorWaitDuration, LOW);
    delay(valveOnDuration);
    handleValveSensor(maxSensorWaitDuration, HIGH);
}

void handleValveSensor(int maxOnDuration, int waitState)
{
    digitalWrite(WATER_PIN, HIGH);
    unsigned long startTime = millis();
    bool sensorSignal = false;

    while (millis() - startTime < (unsigned long)maxOnDuration)
    {
        int sensorState = digitalRead(SENSOR_PIN);

        if (sensorState == waitState)
        {
            sensorSignal = true;
            break;
        }

        delay(10);
    }
    digitalWrite(WATER_PIN, LOW);
}

void resetValve(int maxSensorWaitDuration)
{
    digitalWrite(WATER_PIN, HIGH);
    delay(5000);
    digitalWrite(WATER_PIN, LOW);

    if (digitalRead(SENSOR_PIN) == HIGH)
    {
        handleValveSensor(maxSensorWaitDuration, HIGH);
    }

    if (digitalRead(SENSOR_PIN) == LOW)
    {
        handleValveSensor(maxSensorWaitDuration, HIGH);
        if (digitalRead(SENSOR_PIN) == HIGH)
        {
            handleValveSensor(maxSensorWaitDuration, LOW);
        }
    }
}

void setup()
{
    Serial.begin(115200);
    delay(1000);
    pinMode(WATER_PIN, OUTPUT);
    pinMode(SENSOR_PIN, INPUT_PULLUP);
    resetValve(10000);
    Serial.println("\nEnkel testfil");
}

void loop()
{
    const int WATER_SECONDS = 5;
    const int WAIT_SECONDS = 10;

    handleValveSensor(100000, WATER_SECONDS * 1000);
    delay(WAIT_SECONDS * 1000);
}