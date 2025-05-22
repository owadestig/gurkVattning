#ifndef LEDHANDLER_H
#define LEDHANDLER_H

#include <Arduino.h>

// Abstract interface for GPIO operations (for mocking)
class IGPIOInterface
{
public:
    virtual ~IGPIOInterface() = default;
    virtual void pinMode(uint8_t pin, uint8_t mode) = 0;
    virtual void digitalWrite(uint8_t pin, uint8_t value) = 0;
    virtual int digitalRead(uint8_t pin) = 0;
    virtual void delay(unsigned long ms) = 0;
};

// Real GPIO implementation
class ArduinoGPIOWrapper : public IGPIOInterface
{
public:
    void pinMode(uint8_t pin, uint8_t mode) override
    {
        ::pinMode(pin, mode);
    }

    void digitalWrite(uint8_t pin, uint8_t value) override
    {
        ::digitalWrite(pin, value);
    }

    int digitalRead(uint8_t pin) override
    {
        return ::digitalRead(pin);
    }

    void delay(unsigned long ms) override
    {
        ::delay(ms);
    }
};

enum class LEDState
{
    OFF,
    ON,
    BLINKING,
    ERROR
};

enum class LEDPattern
{
    SOLID_ON,
    SOLID_OFF,
    SLOW_BLINK,
    FAST_BLINK,
    ERROR_PATTERN
};

class LEDHandler
{
private:
    static LEDHandler *instance;
    IGPIOInterface *gpioInterface;
    bool ownsInterface;

    uint8_t ledPin;
    LEDState currentState;
    LEDPattern currentPattern;
    unsigned long lastBlinkTime;
    unsigned long blinkInterval;
    bool ledPhysicalState; // true = ON, false = OFF

    LEDHandler();

public:
    static LEDHandler &getInstance();
    static void setGPIOInterface(IGPIOInterface *interface); // For testing

    ~LEDHandler();

    void initialize(uint8_t pin);
    void setPattern(LEDPattern pattern);
    void turnOn();
    void turnOff();
    void reset();  // Your resetLED() function
    void update(); // Call this in loop() to handle blinking

    // Status methods
    LEDState getState() const;
    LEDPattern getPattern() const;
    bool isOn() const;

    // Specific patterns for your use case
    void showConnecting();
    void showConnected();
    void showError();
    void showWatering();

private:
    void setPhysicalState(bool on);
    void updateBlinking();
};

#endif