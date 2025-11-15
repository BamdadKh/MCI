#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <protocol_UART.h>

static UART debugUart(&PIND, &DDRD, &PORTD, PD1, &PIND, &DDRD, &PORTD, PD0, 9600UL);

static void debugPrintDecimal(uint32_t value) {
    char buf[11];
    uint8_t len = 0;
    do {
        buf[len++] = '0' + (value % 10);
        value /= 10;
    } while (value != 0);
    while (len--) {
        debugUart.sendByte(buf[len]);
    }
}

int main(void) {
    debugUart.begin();
    debugUart.sendString("SerialMonitor example start\r\n");
    debugUart.sendString("Counter: ");
    debugPrintDecimal(0);
    debugUart.sendString("\r\n");

    uint32_t counter = 0;
    while (1) {
        debugUart.sendString("Counter: ");
        debugPrintDecimal(counter++);
        debugUart.sendString("\r\n");
        _delay_ms(1000);
    }

    return 0;
}
