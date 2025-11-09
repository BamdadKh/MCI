#include <Arduino.h>
#include <device_DFPlayerMini.h>
#include <device_SerialMonitor.h>

// Wiring:
// D4 -> DFPlayer RX
// D5 -> DFPlayer TX (optional if not reading responses)
// 5V, GND, Speaker to module
// SD card: 0001.mp3 in root

DFPlayerMini player(&PIND, &DDRD, &PORTD, PD4, &PIND, &DDRD, &PORTD, PD5, 9600);
SerialMonitor Debug;

void setup() {
    sei();
    Debug.begin(9600);
    Debug.println("DFPlayerMini loop example");
    player.begin(false, 1500); // 1.5s settle
    player.setVolume(20);
    Debug.println("Volume set to 20");
    player.loopTrack(1); // loop first track
    Debug.println("Looping track 1");
}

void loop() {
    // Could periodically print status or react to button presses.
}
