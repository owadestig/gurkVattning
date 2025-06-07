#ifndef ARDUINO_MOCK_H
#define ARDUINO_MOCK_H

#include <string>
#include <cstdint>

// Mock Arduino types
typedef std::string String;
typedef bool boolean;

// Mock Arduino constants
#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2

// Mock HTTP status codes
#define HTTP_CODE_OK 200
#define HTTP_CODE_MOVED_PERMANENTLY 301

// Mock Arduino functions
inline void pinMode(uint8_t pin, uint8_t mode)
{
    (void)pin;
    (void)mode;
}
inline void digitalWrite(uint8_t pin, uint8_t value)
{
    (void)pin;
    (void)value;
}
inline int digitalRead(uint8_t pin)
{
    (void)pin;
    return LOW;
}
inline void delay(unsigned long ms) { (void)ms; }
inline unsigned long millis() { return 0; }

// Mock Serial for tests
class MockSerial
{
public:
    void begin(int baud) { (void)baud; }
    void print(const char *str) { (void)str; }
    void println(const char *str) { (void)str; }
    void println() {}
    template <typename T>
    void print(T value) { (void)value; }
    template <typename T>
    void println(T value) { (void)value; }
    template <typename T>
    void printf(const char *format, T value)
    {
        (void)format;
        (void)value;
    }
};

extern MockSerial Serial;

#endif