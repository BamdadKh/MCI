#include <Arduino.h>
#include <device_SerialMonitor.h>

SerialMonitor Debug; // wraps global Serial

void setup() {
    Debug.begin(9600);
    Debug.println("SerialMonitor example start");
    Debug.print("Counter: ");
    Debug.println(0);
}

void loop() {
    static unsigned long last = 0;
    static uint32_t counter = 0;
    if (millis() - last >= 1000) { // every second
        last = millis();
        Debug.print("Counter: ");
        Debug.println(counter++);
    }
}
