#include <avr/io.h>
#include <avr/interrupt.h>
#include <protocol_UART.h>

// Software UART on D5 (TX) and D4 (RX).
// Connect a USB-TTL at 9600 baud to see echoed characters.
static UART softSerial(&PIND, &DDRD, &PORTD, PD5,  // TX
                       &PIND, &DDRD, &PORTD, PD4,  // RX
                       9600UL);

int main(void) {
    sei();
    softSerial.begin();
    softSerial.sendString("UART echo ready\r\n");

    while (1) {
        int c = softSerial.read();
        if (c >= 0) {
            softSerial.sendByte((uint8_t)c); // echo back
        }
    }

    return 0;
}
