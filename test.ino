#include <Arduino.h>

// Define the pin for the digital input
const int inputPin = 2;

// Variable to store the input state
int inputState = LOW;

void setup()
{
    // Set the input pin as an input
    pinMode(inputPin, INPUT);
}

void loop()
{
    // Read the input state
    inputState = digitalRead(inputPin);

    // If the input is HIGH, call the funcB() function
    if (inputState == HIGH)
    {
        funcB();
    }
}

// An empty function called funcB()
void funcB()
{
    // Add your code here if you want to perform any actions
    // when the input is HIGH
}