#include "protocol_UART.h"
#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h> 


UART::UART(volatile uint8_t *tx_pin_reg, volatile uint8_t *tx_ddr, volatile uint8_t *tx_port, uint8_t tx_pin,
             volatile uint8_t *rx_pin_reg, volatile uint8_t *rx_ddr, volatile uint8_t *rx_port, uint8_t rx_pin, unsigned long baud) :
             TX_PIN_REG(tx_pin_reg), TX_DDR(tx_ddr), TX_PORT(tx_port), TX_PIN(tx_pin),
             RX_PIN_REG(rx_pin_reg), RX_DDR(rx_ddr), RX_PORT(rx_port), RX_PIN(rx_pin),
             baudrate(baud) {
                 
    // Configure TX pin as output
    *TX_DDR |= (1 << TX_PIN);
    // Configure RX pin as input
    *RX_DDR &= ~(1 << RX_PIN);
    // Enable pull-up resistor on RX pin
    *RX_PORT |= (1 << RX_PIN);
    
    // Set TX high (idle)
    *TX_PORT |= (1 << TX_PIN);
}

void UART::sendByte(uint8_t data) {
    unsigned long bit_delay_us = 1000000 / baudrate;

    cli(); // Disable interrupts

    // Start bit
    *TX_PORT &= ~(1 << TX_PIN); // Pull TX low
    _delay_us(bit_delay_us); 

    // Data bits (LSB first)
    for (uint8_t i = 0; i < 8; i++) {
        if (data & (1 << i)) {
            *TX_PORT |= (1 << TX_PIN); // Send 1
        } else {
            *TX_PORT &= ~(1 << TX_PIN); // Send 0
        }
        _delay_us(bit_delay_us);
    }

    // Stop bit
    *TX_PORT |= (1 << TX_PIN); // Pull TX high
    _delay_us(bit_delay_us);

    sei(); // Re-enable interrupts
}

void UART::sendBytes(const uint8_t *data, unsigned int length) {
    for (unsigned int i = 0; i < length; i++) {
        sendByte(data[i]);
    }
}

void UART::sendString(const char *str) {
    while (*str) {
        sendByte(static_cast<uint8_t>(*str));
        str++;
    }
}  

