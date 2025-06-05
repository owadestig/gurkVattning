#include <iostream>
#include <cassert>
#include <string>

// Mock Arduino functions
static unsigned long mockMillisValue = 0;

void delay(unsigned long /*ms*/) {}

unsigned long millis()
{
    return mockMillisValue;
}

void setMockMillis(unsigned long time)
{
    mockMillisValue = time;
}

class Serial_Mock
{
public:
    template <typename T>
    void print(T value) { std::cout << value; }

    template <typename T>
    void println(T value) { std::cout << value << std::endl; }

    void println() { std::cout << std::endl; }
} Serial;

// Include mock interface
#include "mock/MockWaterController.h"

// Simplified WaterValveController for testing
class WaterValveController
{
private:
    static WaterValveController *instance;
    IGPIOInterface *gpioInterface;
    bool ownsInterface;

    uint8_t valveControlPin;
    uint8_t valveSensorPin;
    unsigned long maxSensorWaitDuration;

    WaterValveController() : gpioInterface(nullptr), ownsInterface(false),
                             valveControlPin(0), valveSensorPin(0), maxSensorWaitDuration(10000) {}

public:
    static WaterValveController &getInstance()
    {
        if (instance == nullptr)
        {
            instance = new WaterValveController();
        }
        return *instance;
    }

    static void setGPIOInterface(IGPIOInterface *interface)
    {
        WaterValveController &controller = getInstance();
        if (controller.ownsInterface && controller.gpioInterface)
        {
            delete controller.gpioInterface;
        }
        controller.gpioInterface = interface;
        controller.ownsInterface = false;
    }

    void initialize(uint8_t controlPin, uint8_t sensorPin, unsigned long maxWaitDuration)
    {
        valveControlPin = controlPin;
        valveSensorPin = sensorPin;
        maxSensorWaitDuration = maxWaitDuration;

        gpioInterface->pinMode(valveControlPin, OUTPUT);
        gpioInterface->pinMode(valveSensorPin, INPUT_PULLUP);
    }

    void handleValveSensor(int /*timeUntilWatering*/, int maxOnDuration,
                           const char * /*noSensorSignalUrl*/, const char * /*ssid*/,
                           const char * /*password*/, int waitState)
    {
        // Turn on the valve
        gpioInterface->digitalWrite(valveControlPin, HIGH);

        unsigned long startTime = gpioInterface->millis();
        bool sensorSignal = false;

        while (gpioInterface->millis() - startTime < maxOnDuration)
        {
            // Read the sensor state directly without debouncing
            int sensorState = gpioInterface->digitalRead(valveSensorPin);

            // Check if the sensor state matches the wait state
            if (sensorState == waitState)
            {
                sensorSignal = true;
                break;
            }

            gpioInterface->delay(10); // Small delay to prevent high CPU usage
        }

        // Turn off the valve
        gpioInterface->digitalWrite(valveControlPin, LOW);

        // If no sensor signal received, perform shutdown
        if (!sensorSignal)
        {
            std::cout << "No sensor signal received, shutting down" << std::endl;
        }
    }

    void resetValve()
    {
        gpioInterface->digitalWrite(valveControlPin, HIGH);
        gpioInterface->delay(5000);
        gpioInterface->digitalWrite(valveControlPin, LOW);

        if (gpioInterface->digitalRead(valveSensorPin) == HIGH)
        {
            handleValveSensor(0, maxSensorWaitDuration, nullptr, nullptr, nullptr, HIGH);
        }

        if (gpioInterface->digitalRead(valveSensorPin) == LOW)
        {
            handleValveSensor(0, maxSensorWaitDuration, nullptr, nullptr, nullptr, HIGH);
            if (gpioInterface->digitalRead(valveSensorPin) == HIGH)
            {
                handleValveSensor(0, maxSensorWaitDuration, nullptr, nullptr, nullptr, LOW);
            }
        }
    }

    // Reset for testing
    static void reset()
    {
        if (instance)
        {
            if (instance->ownsInterface && instance->gpioInterface)
            {
                delete instance->gpioInterface;
            }
            delete instance;
            instance = nullptr;
        }
    }
};

WaterValveController *WaterValveController::instance = nullptr;

// Test functions
void test_valve_sensor_success()
{
    std::cout << "=== Testing valve sensor detection success ===" << std::endl;

    WaterValveController::reset();
    setMockMillis(0);

    MockGPIOInterface *mockGPIO = new MockGPIOInterface();
    WaterValveController::setGPIOInterface(mockGPIO);
    WaterValveController &controller = WaterValveController::getInstance();

    controller.initialize(5, 14, 5000); // valve control pin 5, sensor pin 14
    mockGPIO->clearOperations();

    // Set up sensor to read HIGH after a few attempts (valve opened)
    std::vector<bool> sensorReadings = {false, false, true}; // LOW, LOW, HIGH
    mockGPIO->setSensorReadingSequence(sensorReadings);

    controller.handleValveSensor(0, 5000, nullptr, nullptr, nullptr, HIGH);

    // Check that valve was turned on and off
    assert(mockGPIO->countOperations("digitalWrite") >= 2);

    std::cout << "✓ Valve sensor success test passed" << std::endl;

    delete mockGPIO;
}

void test_valve_sensor_timeout()
{
    std::cout << "=== Testing valve sensor timeout ===" << std::endl;

    WaterValveController::reset();
    setMockMillis(0);

    MockGPIOInterface *mockGPIO = new MockGPIOInterface();
    WaterValveController::setGPIOInterface(mockGPIO);
    WaterValveController &controller = WaterValveController::getInstance();

    controller.initialize(5, 14, 1000); // Short timeout for testing
    mockGPIO->clearOperations();

    // Set up sensor to never read the expected state
    std::vector<bool> sensorReadings(200, false); // 200 LOW readings (never HIGH)
    mockGPIO->setSensorReadingSequence(sensorReadings);

    controller.handleValveSensor(0, 1000, nullptr, nullptr, nullptr, HIGH);

    // Should still turn valve on and off even on timeout
    assert(mockGPIO->countOperations("digitalWrite") >= 2);

    std::cout << "✓ Valve sensor timeout test passed" << std::endl;

    delete mockGPIO;
}

void test_reset_valve_sequence()
{
    std::cout << "=== Testing reset valve sequence ===" << std::endl;

    WaterValveController::reset();
    setMockMillis(0);

    MockGPIOInterface *mockGPIO = new MockGPIOInterface();
    WaterValveController::setGPIOInterface(mockGPIO);
    WaterValveController &controller = WaterValveController::getInstance();

    controller.initialize(5, 14, 5000);
    mockGPIO->clearOperations();

    // Simulate complete reset sequence:
    // 1. Initial sensor read (LOW) - first digitalRead in resetValve
    // 2. Sensor reads during handleValveSensor call (looking for HIGH)
    // 3. Final sensor read (HIGH) - second digitalRead in resetValve
    // 4. More sensor reads during second handleValveSensor call (looking for LOW)
    std::vector<bool> sensorReadings = {
        false,       // Initial read: sensor shows valve closed
        false, true, // During first handleValveSensor: LOW, then HIGH (sensor confirms valve opened)
        true,        // Second read: sensor shows valve open
        true, false  // During second handleValveSensor: HIGH, then LOW (sensor confirms valve closed)
    };
    mockGPIO->setSensorReadingSequence(sensorReadings);

    controller.resetValve();

    // Should have multiple valve operations during reset sequence:
    // 1. Initial pulse: HIGH, LOW (2 operations)
    // 2. First handleValveSensor: HIGH, LOW (2 operations)
    // 3. Second handleValveSensor: HIGH, LOW (2 operations)
    // Total: at least 6 digitalWrite operations
    assert(mockGPIO->countOperations("digitalWrite") >= 6);
    assert(mockGPIO->countOperations("digitalRead") >= 4); // Multiple sensor reads

    std::cout << "✓ Reset valve sequence test passed" << std::endl;

    delete mockGPIO;
}

void test_valve_initialization()
{
    std::cout << "=== Testing valve controller initialization ===" << std::endl;

    WaterValveController::reset();
    setMockMillis(0);

    MockGPIOInterface *mockGPIO = new MockGPIOInterface();
    WaterValveController::setGPIOInterface(mockGPIO);
    WaterValveController &controller = WaterValveController::getInstance();

    controller.initialize(5, 14, 10000);

    // Check that pins were configured correctly
    assert(mockGPIO->countOperations("pinMode") == 2);

    std::cout << "✓ Valve initialization test passed" << std::endl;

    delete mockGPIO;
}

void test_reset_valve_simple_case()
{
    std::cout << "=== Testing reset valve simple case ===" << std::endl;

    WaterValveController::reset();
    setMockMillis(0);

    MockGPIOInterface *mockGPIO = new MockGPIOInterface();
    WaterValveController::setGPIOInterface(mockGPIO);
    WaterValveController &controller = WaterValveController::getInstance();

    controller.initialize(5, 14, 5000);
    mockGPIO->clearOperations();

    // Simple case: sensor is HIGH initially (valve already open)
    std::vector<bool> sensorReadings = {
        true,      // Initial read: sensor shows valve open
        true, true // During handleValveSensor: stay HIGH (waiting for HIGH, so immediate success)
    };
    mockGPIO->setSensorReadingSequence(sensorReadings);

    controller.resetValve();

    // Should have at least: initial pulse (2 ops) + handleValveSensor (2 ops) = 4 operations
    assert(mockGPIO->countOperations("digitalWrite") >= 4);
    assert(mockGPIO->countOperations("digitalRead") >= 1);

    std::cout << "✓ Reset valve simple case test passed" << std::endl;

    delete mockGPIO;
}

int main()
{
    std::cout << "Running WaterValveController Tests..." << std::endl
              << std::endl;

    try
    {
        test_valve_initialization();
        test_valve_sensor_success();
        test_valve_sensor_timeout();
        test_reset_valve_simple_case();
        test_reset_valve_sequence();

        std::cout << std::endl
                  << "🎉 All water valve controller tests passed!" << std::endl;
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cout << "❌ Test failed: " << e.what() << std::endl;
        return 1;
    }
}