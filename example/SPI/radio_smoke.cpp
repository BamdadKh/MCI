#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>
#include <device_NRF24.h>
#include <protocol_UART.h>

static constexpr bool ROLE_TRANSMITTER = true; // Set false on the receiver board
static constexpr bool REQUEST_ACK = false;     // Enable when both radios are active
static constexpr uint8_t RADIO_CHANNEL = 76;
static constexpr uint8_t PAYLOAD_SIZE = 16;

static const uint8_t PIPE0_ADDRESS[5] = {'N', 'R', 'F', '2', '4'};

static UART debugUart(&PIND, &DDRD, &PORTD, PD1, &PIND, &DDRD, &PORTD, PD0, 115200UL);

static NRF24 radio(
    &PINB, &DDRB, &PORTB, PB3,
    &PINB, &DDRB, &PORTB, PB4,
    &PINB, &DDRB, &PORTB, PB5,
    &PINB, &DDRB, &PORTB, PB2,
    &PIND, &DDRD, &PORTD, PD7
);

static inline void led_init(void) {
    DDRB |= (1 << PB5);
    PORTB &= ~(1 << PB5);
}

static inline void led_set(bool on) {
    if (on) {
        PORTB |= (1 << PB5);
    } else {
        PORTB &= ~(1 << PB5);
    }
}

static inline void led_toggle(void) {
    PORTB ^= (1 << PB5);
}

static void debug_send_hex(uint8_t value) {
    const char digits[] = "0123456789ABCDEF";
    debugUart.sendByte(digits[(value >> 4) & 0x0F]);
    debugUart.sendByte(digits[value & 0x0F]);
}

int main(void) {
    sei();
    led_init();
    led_set(false);
    debugUart.begin();
    debugUart.sendString("nRF24 smoke test starting...\r\n");

    if (!radio.begin(true, RADIO_CHANNEL, PAYLOAD_SIZE)) {
        debugUart.sendString("radio.begin failed\r\n");
    }

    radio.setAutoAck(REQUEST_ACK);
    radio.openWritingPipe(PIPE0_ADDRESS, sizeof(PIPE0_ADDRESS));
    radio.openReadingPipe(0, PIPE0_ADDRESS, sizeof(PIPE0_ADDRESS), REQUEST_ACK);

    if (ROLE_TRANSMITTER) {
        radio.stopListening();
        debugUart.sendString("Configured as transmitter\r\n");
    } else {
        radio.startListening();
        debugUart.sendString("Configured as receiver\r\n");
    }

    led_set(true);

    uint32_t txCounter = 0;
    while (1) {
        if (ROLE_TRANSMITTER) {
            uint8_t payload[PAYLOAD_SIZE] = {0};
            payload[0] = 0x42;
            payload[1] = static_cast<uint8_t>((txCounter >> 24) & 0xFF);
            payload[2] = static_cast<uint8_t>((txCounter >> 16) & 0xFF);
            payload[3] = static_cast<uint8_t>((txCounter >> 8) & 0xFF);
            payload[4] = static_cast<uint8_t>(txCounter & 0xFF);

            bool ok = radio.write(payload, PAYLOAD_SIZE, REQUEST_ACK);
            debugUart.sendString("TX ");
            debugUart.sendString(ok ? "OK" : "FAIL");
            debugUart.sendString(" status=0x");
            debug_send_hex(radio.getStatus());
            debugUart.sendString("\r\n");
            if (!ok && REQUEST_ACK) {
                debugUart.sendString("Hint: enable the peer radio or set REQUEST_ACK=false.\r\n");
            }

            txCounter++;
            _delay_ms(1000);
        } else {
            if (radio.available()) {
                uint8_t buffer[PAYLOAD_SIZE] = {0};
                if (radio.read(buffer, PAYLOAD_SIZE)) {
                    debugUart.sendString("RX payload: ");
                    for (uint8_t i = 0; i < PAYLOAD_SIZE; ++i) {
                        if (buffer[i] < 0x10) {
                            debugUart.sendByte('0');
                        }
                        debug_send_hex(buffer[i]);
                        debugUart.sendByte(' ');
                    }
                    debugUart.sendString("\r\n");
                    led_toggle();
                }
            }
            _delay_ms(50);
        }
    }

    return 0;
}
