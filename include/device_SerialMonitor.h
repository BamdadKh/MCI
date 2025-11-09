#ifndef DEVICE_SERIALMONITOR_H
#define DEVICE_SERIALMONITOR_H

#include <Arduino.h>
#include <stdint.h>
#include <stddef.h>

// Arduino-like Serial wrapper on top of HardwareSerial (USB Serial on Nano)
// Usage example:
//   SerialMonitor Debug; // wraps global Serial
//   Debug.begin(9600);
//   Debug.println("Hello");

class SerialMonitor {
public:
    explicit SerialMonitor(HardwareSerial &ser = Serial) : _ser(ser) {}

    bool begin(unsigned long baud = 9600) { _ser.begin(baud); return true; }

    // TX API
    size_t write(uint8_t b);
    size_t write(const uint8_t *buffer, size_t size);

    void print(const char *s);
    void print(char c);
    void print(int value, uint8_t base = 10);
    void print(unsigned int value, uint8_t base = 10);
    void print(long value, uint8_t base = 10);
    void print(unsigned long value, uint8_t base = 10);
    void print(float value, uint8_t digits = 2);

    void println();
    void println(const char *s);
    void println(char c);
    void println(int value, uint8_t base = 10);
    void println(unsigned int value, uint8_t base = 10);
    void println(long value, uint8_t base = 10);
    void println(unsigned long value, uint8_t base = 10);
    void println(float value, uint8_t digits = 2);

    // RX API passthrough
    int available() const { return _ser.available(); }
    int read() { return _ser.read(); }
    int peek() const { return _ser.peek(); }
    void flush() { _ser.flush(); }

private:
    void _printNumberUnsigned(unsigned long value, uint8_t base);
    HardwareSerial &_ser;
};

#endif // DEVICE_SERIALMONITOR_H
