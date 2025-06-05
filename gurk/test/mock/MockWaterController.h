#ifndef MOCKWATERVALVECONTROLLER_H
#define MOCKWATERVALVECONTROLLER_H

#include <iostream>
#include <vector>
#include <string>

// Mock Arduino constants
#define OUTPUT 1
#define INPUT_PULLUP 2
#define HIGH 1
#define LOW 0

// Mock millis function
unsigned long millis();
void setMockMillis(unsigned long time);

// GPIO operation log entry
struct GPIOOperation
{
    std::string operation;
    int pin;
    int value;
    unsigned long timestamp;

    GPIOOperation(const std::string &op, int p, int val = -1)
        : operation(op), pin(p), value(val), timestamp(millis()) {}
};

// Abstract interface for GPIO operations (for mocking)
class IGPIOInterface
{
public:
    virtual ~IGPIOInterface() = default;
    virtual void pinMode(uint8_t pin, uint8_t mode) = 0;
    virtual void digitalWrite(uint8_t pin, uint8_t value) = 0;
    virtual int digitalRead(uint8_t pin) = 0;
    virtual void delay(unsigned long ms) = 0;
    virtual unsigned long millis() = 0;
};

class MockGPIOInterface : public IGPIOInterface
{
private:
    std::vector<GPIOOperation> operations;
    std::vector<bool> pinStates;
    std::vector<bool> sensorReadings; // Simulated sensor readings
    size_t sensorReadingIndex;

public:
    MockGPIOInterface() : pinStates(20, false), sensorReadingIndex(0) {}

    void pinMode(uint8_t pin, uint8_t mode) override
    {
        operations.push_back(GPIOOperation("pinMode", pin, mode));
        std::cout << "MockGPIO: pinMode(" << (int)pin << ", " << (mode == OUTPUT ? "OUTPUT" : "INPUT_PULLUP") << ")" << std::endl;
    }

    void digitalWrite(uint8_t pin, uint8_t value) override
    {
        operations.push_back(GPIOOperation("digitalWrite", pin, value));
        if (pin < pinStates.size())
        {
            pinStates[pin] = (value == HIGH);
        }
        std::cout << "MockGPIO: digitalWrite(" << (int)pin << ", " << (value == HIGH ? "HIGH" : "LOW") << ") - Valve " << (value == HIGH ? "OPEN" : "CLOSED") << std::endl;
    }

    int digitalRead(uint8_t pin) override
    {
        int value = LOW;

        // Simulate sensor readings if we have a sequence set up
        if (!sensorReadings.empty() && sensorReadingIndex < sensorReadings.size())
        {
            value = sensorReadings[sensorReadingIndex] ? HIGH : LOW;
            sensorReadingIndex++;
        }
        else if (pin < pinStates.size())
        {
            value = pinStates[pin] ? HIGH : LOW;
        }

        operations.push_back(GPIOOperation("digitalRead", pin, value));
        std::cout << "MockGPIO: digitalRead(" << (int)pin << ") -> " << (value == HIGH ? "HIGH" : "LOW") << " (sensor: valve " << (value == HIGH ? "OPEN" : "CLOSED") << ")" << std::endl;
        return value;
    }

    void delay(unsigned long ms) override
    {
        operations.push_back(GPIOOperation("delay", -1, ms));
        std::cout << "MockGPIO: delay(" << ms << "ms)" << std::endl;
        setMockMillis(millis() + ms); // Advance mock time
    }

    unsigned long millis() override
    {
        return ::millis(); // Use the global mock millis
    }

    // Test helpers
    void setSensorReadingSequence(const std::vector<bool> &sequence)
    {
        sensorReadings = sequence;
        sensorReadingIndex = 0;
    }

    void clearOperations()
    {
        operations.clear();
    }

    const std::vector<GPIOOperation> &getOperations() const
    {
        return operations;
    }

    bool getValveControlState(uint8_t pin) const
    {
        return (pin < pinStates.size()) ? pinStates[pin] : false;
    }

    int countOperations(const std::string &opType) const
    {
        int count = 0;
        for (const auto &op : operations)
        {
            if (op.operation == opType)
                count++;
        }
        return count;
    }
};

#endif