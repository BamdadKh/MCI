#include <Arduino.h>
#include <protocol_I2C.h>
#include <device_SerialMonitor.h>

// SDA=A4 (PC4), SCL=A5 (PC5) on Arduino Nano
I2C i2c(&PINC, &DDRC, &PORTC, PC4, &PINC, &DDRC, &PORTC, PC5);
SerialMonitor Debug;

void setup() {
    Debug.begin(9600);
    Debug.println("MPU6050 WHO_AM_I test");

    // Wake device: write 0x00 to PWR_MGMT_1 (0x6B)
    uint8_t cfg[2] = {0x6B, 0x00};
    if (!i2c.writeMessage(0x68, cfg, 2)) {
        Debug.println("Wake write failed");
    }

    // Set register pointer to WHO_AM_I (0x75)
    uint8_t reg = 0x75;
    if (!i2c.writeMessage(0x68, &reg, 1)) {
        Debug.println("WHO_AM_I pointer write failed");
    }

    // Read one byte
    uint8_t whoami = 0xFF;
    if (!i2c.readMessage(0x68, &whoami, 1)) {
        Debug.println("Read failed");
    } else {
        Debug.print("WHO_AM_I: 0x");
        Debug.println((int)whoami, 16);
    }
}

void loop() {
    // No repeating logic; could poll sensor here.
}
