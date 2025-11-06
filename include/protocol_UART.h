#include <stdint.h>
#include <avr/io.h>

class UART {

public:
    UART(volatile uint8_t *tx_pin_reg, volatile uint8_t *tx_ddr, volatile uint8_t *tx_port, uint8_t tx_pin,
         volatile uint8_t *rx_pin_reg, volatile uint8_t *rx_ddr, volatile uint8_t *rx_port, uint8_t rx_pin, unsigned long baud);

    UART() = delete;
    
    void sendByte(uint8_t data);
    void sendBytes(const uint8_t *data, unsigned int length);
    void sendString(const char *str);

    void handleReceiveInterrupt(void (*callback)(uint8_t));

private: 
    // Registers for TX
    volatile uint8_t *TX_PIN_REG;
    volatile uint8_t *TX_DDR;
    volatile uint8_t *TX_PORT;
    uint8_t TX_PIN;

    // Registers for RX
    volatile uint8_t *RX_PIN_REG;
    volatile uint8_t *RX_DDR;
    volatile uint8_t *RX_PORT;
    uint8_t RX_PIN;

    unsigned long baudrate;
};