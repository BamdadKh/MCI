#include <Arduino.h>
#include <device_NRF24.h>
#include <device_SerialMonitor.h>

static constexpr bool ROLE_TRANSMITTER = true; // Set false on the receiver board
static constexpr bool REQUEST_ACK = false;     // Enable when both radios are active
static constexpr uint8_t RADIO_CHANNEL = 76;
static constexpr uint8_t PAYLOAD_SIZE = 16;

static const uint8_t PIPE0_ADDRESS[5] = {'N', 'R', 'F', '2', '4'};

static SerialMonitor Debug;

static NRF24 radio(
    &PINB, &DDRB, &PORTB, PB3,
    &PINB, &DDRB, &PORTB, PB4,
    &PINB, &DDRB, &PORTB, PB5,
    &PINB, &DDRB, &PORTB, PB2,
    &PIND, &DDRD, &PORTD, PD7
);

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    Debug.begin(115200);
    Debug.println("nRF24 smoke test starting...");

    if (!radio.begin(true, RADIO_CHANNEL, PAYLOAD_SIZE)) {
        Debug.println("radio.begin failed");
    }

    radio.setAutoAck(REQUEST_ACK);
    radio.openWritingPipe(PIPE0_ADDRESS, sizeof(PIPE0_ADDRESS));
    radio.openReadingPipe(0, PIPE0_ADDRESS, sizeof(PIPE0_ADDRESS), REQUEST_ACK);

    if (ROLE_TRANSMITTER) {
        radio.stopListening();
        Debug.println("Configured as transmitter");
    } else {
        radio.startListening();
        Debug.println("Configured as receiver");
    }

    digitalWrite(LED_BUILTIN, HIGH);
}

void loop() {
    static uint32_t lastAction = 0;

    if (ROLE_TRANSMITTER) {
        uint32_t now = millis();
        if (now - lastAction >= 1000) {
            lastAction = now;

            uint8_t payload[PAYLOAD_SIZE] = {0};
            payload[0] = 0x42;
            payload[1] = static_cast<uint8_t>((now >> 24) & 0xFF);
            payload[2] = static_cast<uint8_t>((now >> 16) & 0xFF);
            payload[3] = static_cast<uint8_t>((now >> 8) & 0xFF);
            payload[4] = static_cast<uint8_t>(now & 0xFF);

            bool ok = radio.write(payload, PAYLOAD_SIZE, REQUEST_ACK);
            Debug.print("TX ");
            Debug.print(ok ? "OK" : "FAIL");
            Debug.print(" status=0x");
            Debug.println(radio.getStatus(), HEX);
            if (!ok && REQUEST_ACK) {
                Debug.println("Hint: enable the peer radio or set REQUEST_ACK=false.");
            }
        }
    } else {
        if (radio.available()) {
            uint8_t buffer[PAYLOAD_SIZE] = {0};
            if (radio.read(buffer, PAYLOAD_SIZE)) {
                Debug.print("RX payload: ");
                for (uint8_t i = 0; i < PAYLOAD_SIZE; ++i) {
                    if (buffer[i] < 0x10) {
                        Debug.print('0');
                    }
                    Debug.print(buffer[i], HEX);
                    Debug.print(' ');
                }
                Debug.println();
                digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
            }
        }
    }
}
