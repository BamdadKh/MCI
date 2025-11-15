#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <protocol_UART.h>
#include <device_DFPlayerMini.h>

// Wiring:
// D4 -> DFPlayer RX
// D5 -> DFPlayer TX (optional if not reading responses)
// 5V, GND, Speaker to module
// SD card: 0001.mp3 in root

static UART debugUart(&PIND, &DDRD, &PORTD, PD1, &PIND, &DDRD, &PORTD, PD0, 9600UL);
static DFPlayerMini player(&PIND, &DDRD, &PORTD, PD4, &PIND, &DDRD, &PORTD, PD5, 9600UL);

int main(void) {
    sei();
    debugUart.begin();
    debugUart.sendString("DFPlayerMini loop example\r\n");

    player.begin(false, 1500); // 1.5s settle
    player.setVolume(20);
    debugUart.sendString("Volume set to 20\r\n");
    player.loopTrack(1); // loop first track
    debugUart.sendString("Looping track 1\r\n");

    while (1) {
        // Idle loop; DFPlayer runs independently
    }

    return 0;
}
