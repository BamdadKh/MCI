#include <avr/io.h>
#include <avr/interrupt.h>
#include <protocol_I2C.h>
#include <protocol_UART.h>

// SDA=A4 (PC4), SCL=A5 (PC5) on Arduino Nano
static I2C i2c(&PINC, &DDRC, &PORTC, PC4, &PINC, &DDRC, &PORTC, PC5);
static UART debugUart(&PIND, &DDRD, &PORTD, PD1, &PIND, &DDRD, &PORTD, PD0, 9600UL);

static void debugSendHex(uint8_t byte) {
    const char hexDigits[] = "0123456789ABCDEF";
    debugUart.sendByte(hexDigits[(byte >> 4) & 0x0F]);
    debugUart.sendByte(hexDigits[byte & 0x0F]);
}

int main(void) {
    sei();
    debugUart.begin();
    debugUart.sendString("MPU6050 WHO_AM_I test\r\n");

    const uint8_t cfg[2] = {0x6B, 0x00};
    if (!i2c.writeMessage(0x68, cfg, 2)) {
        debugUart.sendString("Wake write failed\r\n");
    }

    uint8_t reg = 0x75;
    if (!i2c.writeMessage(0x68, &reg, 1)) {
        debugUart.sendString("WHO_AM_I pointer write failed\r\n");
    }

    uint8_t whoami = 0xFF;
    if (!i2c.readMessage(0x68, &whoami, 1)) {
        debugUart.sendString("Read failed\r\n");
    } else {
        debugUart.sendString("WHO_AM_I: 0x");
        debugSendHex(whoami);
        debugUart.sendString("\r\n");
    }

    while (1) {
        // Placeholder for periodic polling if desired.
    }

    return 0;
}
