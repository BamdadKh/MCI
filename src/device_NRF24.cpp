#include <device_NRF24.h>
#include <util/delay.h>

NRF24::NRF24(volatile uint8_t *mosi_pin_reg, volatile uint8_t *mosi_ddr, volatile uint8_t *mosi_port, uint8_t mosi_pin,
             volatile uint8_t *miso_pin_reg, volatile uint8_t *miso_ddr, volatile uint8_t *miso_port, uint8_t miso_pin,
             volatile uint8_t *sck_pin_reg, volatile uint8_t *sck_ddr, volatile uint8_t *sck_port, uint8_t sck_pin,
             volatile uint8_t *csn_pin_reg, volatile uint8_t *csn_ddr, volatile uint8_t *csn_port, uint8_t csn_pin,
             volatile uint8_t *ce_pin_reg, volatile uint8_t *ce_ddr, volatile uint8_t *ce_port, uint8_t ce_pin,
             volatile uint8_t *irq_pin_reg, volatile uint8_t *irq_ddr, volatile uint8_t *irq_port, uint8_t irq_pin)
    : SPI(mosi_pin_reg, mosi_ddr, mosi_port, mosi_pin,
          miso_pin_reg, miso_ddr, miso_port, miso_pin,
          sck_pin_reg, sck_ddr, sck_port, sck_pin,
          csn_pin_reg, csn_ddr, csn_port, csn_pin),
      CE_PIN_REG(ce_pin_reg), CE_DDR(ce_ddr), CE_PORT(ce_port), CE_PIN(ce_pin),
      CE_MASK(static_cast<uint8_t>(1U << ce_pin)),
      IRQ_PIN_REG(irq_pin_reg), IRQ_DDR(irq_ddr), IRQ_PORT(irq_port), IRQ_PIN(irq_pin),
      IRQ_MASK(irq_pin < 8 ? static_cast<uint8_t>(1U << irq_pin) : 0),
      hasIrqPin(irq_pin_reg != nullptr && irq_ddr != nullptr && irq_port != nullptr) {
}

bool NRF24::begin(bool enableAutoAck, uint8_t channel, uint8_t payloadSizeParam) {
    SPI::begin(false);
    setAutoChipSelect(false);
    setBitOrder(BitOrder::MSBFirst);
    setDataMode(0);
    setClockHz(4000000);

    if (CE_DDR) {
        (*CE_DDR) |= CE_MASK;
    }
    driveCe(false);

    if (hasIrqPin) {
        (*IRQ_DDR) &= static_cast<uint8_t>(~IRQ_MASK);
        if (IRQ_PORT) {
            (*IRQ_PORT) |= IRQ_MASK; // enable pull-up for IRQ
        }
    }

    _delay_ms(5);

    enabledRxPipes = 0x03; // enable pipes 0 and 1 by default
    autoAckEnabled = enableAutoAck;
    autoAckMask = enableAutoAck ? 0x3F : 0x00;

    uint8_t config = CONFIG_EN_CRC | CONFIG_CRCO;
    writeRegister(REG_CONFIG, config);

    updateAutoAckMask();
    writeRegister(REG_EN_RXADDR, enabledRxPipes);
    writeRegister(REG_SETUP_AW, 0x03); // 5-byte address width
    writeRegister(REG_SETUP_RETR, static_cast<uint8_t>((0x04 << 4) | 0x0F)); // 1250us, 15 retries

    setChannel(channel);
    setDataRate(DataRate::Rate1Mbps);
    setPowerLevel(PowerLevel::Max);
    setPayloadSize(payloadSizeParam);

    dynamicPayloads = false;
    configureFeatureRegister();

    flushRx();
    flushTx();
    clearInterrupts(true, true, true);

    powerUp();
    stopListening();

    return true;
}

void NRF24::setAutoAck(bool enabled) {
    autoAckEnabled = enabled;
    autoAckMask = enabled ? 0x3F : 0x00;
    updateAutoAckMask();
    configureFeatureRegister();
}

bool NRF24::setChannel(uint8_t channel) {
    if (channel > 125) {
        return false;
    }
    return writeRegister(REG_RF_CH, channel);
}

bool NRF24::setPayloadSize(uint8_t size) {
    if (size == 0) {
        size = 1;
    }
    if (size > MAX_PAYLOAD_SIZE) {
        size = MAX_PAYLOAD_SIZE;
    }

    payloadSize = size;
    for (uint8_t pipe = 0; pipe < 6; ++pipe) {
        writeRegister(static_cast<uint8_t>(REG_RX_PW_P0 + pipe), payloadSize);
    }
    return true;
}

bool NRF24::setDataRate(DataRate rate) {
    uint8_t setup = readRegister(REG_RF_SETUP);
    setup &= static_cast<uint8_t>(~((1 << 5) | (1 << 3))); // clear RF_DR_LOW and RF_DR_HIGH

    switch (rate) {
        case DataRate::Rate250Kbps:
            setup |= static_cast<uint8_t>(1 << 5);
            break;
        case DataRate::Rate1Mbps:
            // already cleared
            break;
        case DataRate::Rate2Mbps:
            setup |= static_cast<uint8_t>(1 << 3);
            break;
        default:
            break;
    }

    return writeRegister(REG_RF_SETUP, setup);
}

void NRF24::setPowerLevel(PowerLevel level) {
    uint8_t setup = readRegister(REG_RF_SETUP);
    setup &= static_cast<uint8_t>(~(0x03 << 1));

    uint8_t value = 0;
    switch (level) {
        case PowerLevel::Min:
            value = 0x00;
            break;
        case PowerLevel::Low:
            value = 0x02;
            break;
        case PowerLevel::High:
            value = 0x04;
            break;
        case PowerLevel::Max:
        default:
            value = 0x06;
            break;
    }

    setup |= value;
    writeRegister(REG_RF_SETUP, setup);
}

void NRF24::powerUp() {
    uint8_t config = readRegister(REG_CONFIG);
    if ((config & CONFIG_PWR_UP) == 0) {
        writeRegister(REG_CONFIG, static_cast<uint8_t>(config | CONFIG_PWR_UP));
        _delay_ms(2);
    }
}

void NRF24::powerDown() {
    driveCe(false);
    uint8_t config = readRegister(REG_CONFIG);
    if (config & CONFIG_PWR_UP) {
        writeRegister(REG_CONFIG, static_cast<uint8_t>(config & ~CONFIG_PWR_UP));
        _delay_ms(2);
    }
}

void NRF24::startListening() {
    powerUp();
    uint8_t config = readRegister(REG_CONFIG);
    config |= CONFIG_PRIM_RX;
    writeRegister(REG_CONFIG, config);
    clearInterrupts(true, true, true);
    flushRx();
    _delay_us(130);
    driveCe(true);
}

void NRF24::stopListening() {
    driveCe(false);
    uint8_t config = readRegister(REG_CONFIG);
    config &= static_cast<uint8_t>(~CONFIG_PRIM_RX);
    writeRegister(REG_CONFIG, config);
    _delay_us(130);
}

bool NRF24::available() {
    uint8_t status = getStatus();
    if (status & STATUS_RX_DR) {
        return true;
    }
    uint8_t fifo = readRegister(REG_FIFO_STATUS);
    return (fifo & FIFO_STATUS_RX_EMPTY) == 0;
}

bool NRF24::read(void *buffer, uint8_t length) {
    if (buffer == nullptr || length == 0) {
        return false;
    }

    if (!available()) {
        return false;
    }

    uint8_t expectedLength = payloadSize;
    if (dynamicPayloads) {
        expectedLength = readPayloadWidth();
        if (expectedLength > MAX_PAYLOAD_SIZE) {
            flushRx();
            clearInterrupts(false, true, false);
            return false;
        }
    }

    if (expectedLength == 0) {
        expectedLength = payloadSize;
    }

    select();
    SPI::transferByte(CMD_R_RX_PAYLOAD);
    uint8_t *byteBuffer = static_cast<uint8_t *>(buffer);
    for (uint8_t i = 0; i < expectedLength; ++i) {
        uint8_t value = SPI::transferByte(0xFF);
        if (i < length) {
            byteBuffer[i] = value;
        }
    }
    deselect();

    clearInterrupts(false, true, false);

    if (dynamicPayloads && expectedLength > length) {
        flushRx();
        return false;
    }

    return true;
}

bool NRF24::write(const void *buffer, uint8_t length, bool requestAck) {
    if (buffer == nullptr || length == 0 || length > MAX_PAYLOAD_SIZE) {
        return false;
    }

    if (!dynamicPayloads && length != payloadSize) {
        return false;
    }

    uint8_t configBefore = readRegister(REG_CONFIG);
    bool wasListening = (configBefore & CONFIG_PRIM_RX) != 0;
    if (wasListening) {
        stopListening();
    } else {
        driveCe(false);
    }

    clearInterrupts(true, true, true);

    uint8_t fifo = readRegister(REG_FIFO_STATUS);
    if (fifo & FIFO_STATUS_TX_FULL) {
        flushTx();
    }

    select();
    SPI::transferByte(requestAck ? CMD_W_TX_PAYLOAD : CMD_W_TX_PAYLOAD_NOACK);
    const uint8_t *dataBytes = static_cast<const uint8_t *>(buffer);
    for (uint8_t i = 0; i < length; ++i) {
        SPI::transferByte(dataBytes[i]);
    }
    deselect();

    pulseCeHigh(CE_PULSE_US);

    uint16_t waitLoops = 2000;
    while (waitLoops--) {
        uint8_t status = getStatus();
        if (status & STATUS_TX_DS) {
            clearInterrupts(true, false, false);
            if (wasListening) {
                startListening();
            } else {
                driveCe(false);
            }
            return true;
        }
        if (status & STATUS_MAX_RT) {
            clearInterrupts(false, false, true);
            flushTx();
            if (wasListening) {
                startListening();
            } else {
                driveCe(false);
            }
            return false;
        }
        _delay_us(50);
    }

    flushTx();
    if (wasListening) {
        startListening();
    } else {
        driveCe(false);
    }
    return false;
}

void NRF24::openWritingPipe(const uint8_t *address, uint8_t length) {
    if (address == nullptr) {
        return;
    }

    if (length < 3) {
        length = 3;
    }
    if (length > 5) {
        length = 5;
    }

    uint8_t aw = static_cast<uint8_t>(length - 2);
    writeRegister(REG_SETUP_AW, static_cast<uint8_t>(aw & 0x03));

    writeRegister(REG_TX_ADDR, address, length);
    writeRegister(REG_RX_ADDR_P0, address, length);
}

void NRF24::openReadingPipe(uint8_t pipe, const uint8_t *address, uint8_t length, bool enableAutoAckPipe) {
    if (pipe > 5 || address == nullptr) {
        return;
    }

    uint8_t addressWidth = static_cast<uint8_t>((readRegister(REG_SETUP_AW) & 0x03) + 2);
    if (addressWidth < 3) {
        addressWidth = 3;
    }
    if (addressWidth > 5) {
        addressWidth = 5;
    }

    if (pipe < 2) {
        if (length < addressWidth) {
            length = addressWidth;
        }
        if (length > addressWidth) {
            length = addressWidth;
        }

        writeRegister(static_cast<uint8_t>(REG_RX_ADDR_P0 + pipe), address, length);
    } else {
        uint8_t firstByte = address[0];
        writeRegister(static_cast<uint8_t>(REG_RX_ADDR_P0 + pipe), &firstByte, 1);
    }

    enabledRxPipes |= static_cast<uint8_t>(1U << pipe);
    writeRegister(REG_EN_RXADDR, enabledRxPipes);

    if (enableAutoAckPipe) {
        autoAckMask |= static_cast<uint8_t>(1U << pipe);
    } else {
        autoAckMask &= static_cast<uint8_t>(~(1U << pipe));
    }
    if (autoAckMask != 0) {
        autoAckEnabled = true;
    }
    updateAutoAckMask();

    if (!dynamicPayloads) {
        writeRegister(static_cast<uint8_t>(REG_RX_PW_P0 + pipe), payloadSize);
    }

    configureFeatureRegister();
}

uint8_t NRF24::getStatus() {
    return writeCommand(CMD_NOP);
}

void NRF24::clearInterrupts(bool tx, bool rx, bool maxRetry) {
    uint8_t mask = 0;
    if (rx) {
        mask |= STATUS_RX_DR;
    }
    if (tx) {
        mask |= STATUS_TX_DS;
    }
    if (maxRetry) {
        mask |= STATUS_MAX_RT;
    }
    if (mask) {
        writeRegister(REG_STATUS, mask);
    }
}

void NRF24::flushTx() {
    writeCommand(CMD_FLUSH_TX);
}

void NRF24::flushRx() {
    writeCommand(CMD_FLUSH_RX);
}

uint8_t NRF24::writeCommand(uint8_t command, const uint8_t *data, uint8_t length) {
    select();
    uint8_t status = SPI::transferByte(command);
    for (uint8_t i = 0; i < length; ++i) {
        SPI::transferByte(data ? data[i] : 0xFF);
    }
    deselect();
    return status;
}

bool NRF24::writeRegister(uint8_t reg, uint8_t value) {
    select();
    SPI::transferByte(static_cast<uint8_t>(CMD_W_REGISTER | (reg & 0x1F)));
    SPI::transferByte(value);
    deselect();
    return true;
}

bool NRF24::writeRegister(uint8_t reg, const uint8_t *data, uint8_t length) {
    if (data == nullptr || length == 0) {
        return false;
    }
    select();
    SPI::transferByte(static_cast<uint8_t>(CMD_W_REGISTER | (reg & 0x1F)));
    for (uint8_t i = 0; i < length; ++i) {
        SPI::transferByte(data[i]);
    }
    deselect();
    return true;
}

uint8_t NRF24::readRegister(uint8_t reg) {
    select();
    SPI::transferByte(static_cast<uint8_t>(CMD_R_REGISTER | (reg & 0x1F)));
    uint8_t value = SPI::transferByte(0xFF);
    deselect();
    return value;
}

void NRF24::readRegister(uint8_t reg, uint8_t *data, uint8_t length) {
    if (data == nullptr || length == 0) {
        return;
    }
    select();
    SPI::transferByte(static_cast<uint8_t>(CMD_R_REGISTER | (reg & 0x1F)));
    for (uint8_t i = 0; i < length; ++i) {
        data[i] = SPI::transferByte(0xFF);
    }
    deselect();
}

void NRF24::driveCe(bool high) {
    if (!CE_PORT) {
        return;
    }

    if (high) {
        (*CE_PORT) |= CE_MASK;
    } else {
        (*CE_PORT) &= static_cast<uint8_t>(~CE_MASK);
    }
}

void NRF24::pulseCeHigh(uint16_t microseconds) {
    driveCe(true);
    while (microseconds--) {
        _delay_us(1);
    }
    driveCe(false);
}

uint8_t NRF24::readPayloadWidth() {
    select();
    SPI::transferByte(CMD_R_RX_PL_WID);
    uint8_t width = SPI::transferByte(0xFF);
    deselect();
    return width;
}

void NRF24::configureFeatureRegister() {
    uint8_t desired = 0;
    if (dynamicPayloads) {
        desired |= static_cast<uint8_t>((1 << 2) | (1 << 1));
        if (!autoAckEnabled) {
            desired |= 1 << 0;
        }
    } else {
        if (!autoAckEnabled) {
            desired |= 1 << 0;
        }
    }

    writeRegister(REG_FEATURE, desired);
    if (readRegister(REG_FEATURE) != desired) {
        uint8_t activateData = 0x73;
        writeCommand(CMD_ACTIVATE, &activateData, 1);
        writeRegister(REG_FEATURE, desired);
    }

    uint8_t dynpd = dynamicPayloads ? static_cast<uint8_t>(enabledRxPipes & 0x3F) : 0x00;
    writeRegister(REG_DYNPD, dynpd);
}

void NRF24::updateAutoAckMask() {
    uint8_t mask = autoAckEnabled ? static_cast<uint8_t>((autoAckMask & enabledRxPipes) & 0x3F) : 0x00;
    writeRegister(REG_EN_AA, mask);
}
