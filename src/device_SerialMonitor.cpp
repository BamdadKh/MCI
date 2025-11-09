#include <device_SerialMonitor.h>

size_t SerialMonitor::write(uint8_t b) {
    return _ser.write(b);
}

size_t SerialMonitor::write(const uint8_t *buffer, size_t size) {
    return _ser.write(buffer, size);
}

void SerialMonitor::print(const char *s) {
    if (!s) return;
    while (*s) {
        write((uint8_t)*s++);
    }
}

void SerialMonitor::print(char c) { write((uint8_t)c); }

void SerialMonitor::_printNumberUnsigned(unsigned long value, uint8_t base) {
    if (base < 2) base = 10;
    char buf[32];
    uint8_t i = 0;
    do {
        uint8_t digit = (uint8_t)(value % base);
        buf[i++] = (digit < 10) ? ('0' + digit) : ('A' + digit - 10);
        value /= base;
    } while (value && i < sizeof(buf));
    while (i) {
        write((uint8_t)buf[--i]);
    }
}

void SerialMonitor::print(int value, uint8_t base) { 
    if (base == 10 && value < 0) { write('-'); _printNumberUnsigned((unsigned long)(-value), base); }
    else { _printNumberUnsigned((unsigned long)value, base); }
}
void SerialMonitor::print(unsigned int value, uint8_t base) { _printNumberUnsigned(value, base); }
void SerialMonitor::print(long value, uint8_t base) {
    if (base == 10 && value < 0) { write('-'); _printNumberUnsigned((unsigned long)(-value), base); }
    else { _printNumberUnsigned((unsigned long)value, base); }
}
void SerialMonitor::print(unsigned long value, uint8_t base) { _printNumberUnsigned(value, base); }

void SerialMonitor::print(float value, uint8_t digits) {
    if (value < 0) { write('-'); value = -value; }
    unsigned long intPart = (unsigned long)value;
    float remainder = value - (float)intPart;
    _printNumberUnsigned(intPart, 10);
    if (digits) {
        write('.');
        for (uint8_t i = 0; i < digits; i++) {
            remainder *= 10.0f;
            uint8_t d = (uint8_t)remainder;
            write((uint8_t)('0' + d));
            remainder -= d;
        }
    }
}

void SerialMonitor::println() { _ser.write('\r'); _ser.write('\n'); }
void SerialMonitor::println(const char *s) { print(s); println(); }
void SerialMonitor::println(char c) { print(c); println(); }
void SerialMonitor::println(int value, uint8_t base) { print(value, base); println(); }
void SerialMonitor::println(unsigned int value, uint8_t base) { print(value, base); println(); }
void SerialMonitor::println(long value, uint8_t base) { print(value, base); println(); }
void SerialMonitor::println(unsigned long value, uint8_t base) { print(value, base); println(); }
void SerialMonitor::println(float value, uint8_t digits) { print(value, digits); println(); }
