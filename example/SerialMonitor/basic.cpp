#include <Arduino.h>
#include <device_SerialMonitor.h>

SerialMonitor Debug;

int main(void) {
    init();  // Initialize Arduino core
    Debug.begin(9600);
    Debug.println("SerialMonitor example start");
    Debug.print("Counter: ");
    Debug.println(0);

    uint32_t counter = 1;
    while (1) {
        Debug.print("Counter: ");
        Debug.println(counter++);
        delay(1000);
    }

    return 0;
}
