#ifndef DEVICE_NRF24_H
#define DEVICE_NRF24_H

#include <stdint.h>
#include <stddef.h>
#include <protocol_SPI.h>

class NRF24 : protected SPI {
public:
    static constexpr uint8_t MAX_PAYLOAD_SIZE = 32;

    enum class DataRate : uint8_t {
        Rate250Kbps,
        Rate1Mbps,
        Rate2Mbps
    };

    enum class PowerLevel : uint8_t {
        Min,
        Low,
        High,
        Max
    };

    NRF24(volatile uint8_t *mosi_pin_reg, volatile uint8_t *mosi_ddr, volatile uint8_t *mosi_port, uint8_t mosi_pin,
          volatile uint8_t *miso_pin_reg, volatile uint8_t *miso_ddr, volatile uint8_t *miso_port, uint8_t miso_pin,
          volatile uint8_t *sck_pin_reg, volatile uint8_t *sck_ddr, volatile uint8_t *sck_port, uint8_t sck_pin,
          volatile uint8_t *csn_pin_reg, volatile uint8_t *csn_ddr, volatile uint8_t *csn_port, uint8_t csn_pin,
          volatile uint8_t *ce_pin_reg, volatile uint8_t *ce_ddr, volatile uint8_t *ce_port, uint8_t ce_pin,
          volatile uint8_t *irq_pin_reg = nullptr, volatile uint8_t *irq_ddr = nullptr, volatile uint8_t *irq_port = nullptr, uint8_t irq_pin = 0);

    NRF24() = delete;

    bool begin(bool enableAutoAck = true, uint8_t channel = 76, uint8_t payloadSize = MAX_PAYLOAD_SIZE);

    void setAutoAck(bool enabled);
    bool setChannel(uint8_t channel);
    bool setPayloadSize(uint8_t size);
    bool setDataRate(DataRate rate);
    void setPowerLevel(PowerLevel level);

    void powerUp();
    void powerDown();

    void startListening();
    void stopListening();

    bool available();
    bool read(void *buffer, uint8_t length);
    bool write(const void *buffer, uint8_t length, bool requestAck = true);

    void openWritingPipe(const uint8_t *address, uint8_t length);
    void openReadingPipe(uint8_t pipe, const uint8_t *address, uint8_t length, bool enableAutoAck = true);

    uint8_t getStatus();
    void clearInterrupts(bool tx = true, bool rx = true, bool maxRetry = true);
    void flushTx();
    void flushRx();

private:
    uint8_t writeCommand(uint8_t command, const uint8_t *data = nullptr, uint8_t length = 0);
    bool writeRegister(uint8_t reg, uint8_t value);
    bool writeRegister(uint8_t reg, const uint8_t *data, uint8_t length);
    uint8_t readRegister(uint8_t reg);
    void readRegister(uint8_t reg, uint8_t *data, uint8_t length);

    void driveCe(bool high);
    void pulseCeHigh(uint16_t microseconds = 15);

    uint8_t readPayloadWidth();

    void configureFeatureRegister();
    void updateAutoAckMask();

    volatile uint8_t *CE_PIN_REG;
    volatile uint8_t *CE_DDR;
    volatile uint8_t *CE_PORT;
    uint8_t CE_PIN;
    uint8_t CE_MASK;

    volatile uint8_t *IRQ_PIN_REG;
    volatile uint8_t *IRQ_DDR;
    volatile uint8_t *IRQ_PORT;
    uint8_t IRQ_PIN;
    uint8_t IRQ_MASK;
    bool hasIrqPin;

    bool dynamicPayloads = false;
    uint8_t payloadSize = MAX_PAYLOAD_SIZE;
    uint8_t enabledRxPipes = 0x03;
    bool autoAckEnabled = true;
    uint8_t autoAckMask = 0x3F;

    static constexpr uint8_t CMD_R_REGISTER = 0x00;
    static constexpr uint8_t CMD_W_REGISTER = 0x20;
    static constexpr uint8_t CMD_R_RX_PAYLOAD = 0x61;
    static constexpr uint8_t CMD_W_TX_PAYLOAD = 0xA0;
    static constexpr uint8_t CMD_W_TX_PAYLOAD_NOACK = 0xB0;
    static constexpr uint8_t CMD_FLUSH_TX = 0xE1;
    static constexpr uint8_t CMD_FLUSH_RX = 0xE2;
    static constexpr uint8_t CMD_REUSE_TX_PL = 0xE3;
    static constexpr uint8_t CMD_ACTIVATE = 0x50;
    static constexpr uint8_t CMD_R_RX_PL_WID = 0x60;
    static constexpr uint8_t CMD_W_ACK_PAYLOAD = 0xA8;
    static constexpr uint8_t CMD_NOP = 0xFF;

    static constexpr uint8_t REG_CONFIG = 0x00;
    static constexpr uint8_t REG_EN_AA = 0x01;
    static constexpr uint8_t REG_EN_RXADDR = 0x02;
    static constexpr uint8_t REG_SETUP_AW = 0x03;
    static constexpr uint8_t REG_SETUP_RETR = 0x04;
    static constexpr uint8_t REG_RF_CH = 0x05;
    static constexpr uint8_t REG_RF_SETUP = 0x06;
    static constexpr uint8_t REG_STATUS = 0x07;
    static constexpr uint8_t REG_OBSERVE_TX = 0x08;
    static constexpr uint8_t REG_RPD = 0x09;
    static constexpr uint8_t REG_RX_ADDR_P0 = 0x0A;
    static constexpr uint8_t REG_RX_ADDR_P1 = 0x0B;
    static constexpr uint8_t REG_RX_ADDR_P2 = 0x0C;
    static constexpr uint8_t REG_RX_ADDR_P3 = 0x0D;
    static constexpr uint8_t REG_RX_ADDR_P4 = 0x0E;
    static constexpr uint8_t REG_RX_ADDR_P5 = 0x0F;
    static constexpr uint8_t REG_TX_ADDR = 0x10;
    static constexpr uint8_t REG_RX_PW_P0 = 0x11;
    static constexpr uint8_t REG_RX_PW_P1 = 0x12;
    static constexpr uint8_t REG_RX_PW_P2 = 0x13;
    static constexpr uint8_t REG_RX_PW_P3 = 0x14;
    static constexpr uint8_t REG_RX_PW_P4 = 0x15;
    static constexpr uint8_t REG_RX_PW_P5 = 0x16;
    static constexpr uint8_t REG_FIFO_STATUS = 0x17;
    static constexpr uint8_t REG_DYNPD = 0x1C;
    static constexpr uint8_t REG_FEATURE = 0x1D;

    static constexpr uint8_t CONFIG_MASK_RX_DR = 1 << 6;
    static constexpr uint8_t CONFIG_MASK_TX_DS = 1 << 5;
    static constexpr uint8_t CONFIG_MASK_MAX_RT = 1 << 4;
    static constexpr uint8_t CONFIG_EN_CRC = 1 << 3;
    static constexpr uint8_t CONFIG_CRCO = 1 << 2;
    static constexpr uint8_t CONFIG_PWR_UP = 1 << 1;
    static constexpr uint8_t CONFIG_PRIM_RX = 1 << 0;

    static constexpr uint8_t STATUS_RX_DR = 1 << 6;
    static constexpr uint8_t STATUS_TX_DS = 1 << 5;
    static constexpr uint8_t STATUS_MAX_RT = 1 << 4;

    static constexpr uint8_t FIFO_STATUS_RX_EMPTY = 1 << 0;
    static constexpr uint8_t FIFO_STATUS_RX_FULL = 1 << 1;
    static constexpr uint8_t FIFO_STATUS_TX_EMPTY = 1 << 4;
    static constexpr uint8_t FIFO_STATUS_TX_FULL = 1 << 5;

    static constexpr uint16_t CE_PULSE_US = 15;
};

#endif // DEVICE_NRF24_H
