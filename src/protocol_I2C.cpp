#include "protocol_I2C.h"
#include <util/delay.h>

// ---------- Constructors ----------
// Preferred explicit constructor (PIN register provided)
I2C::I2C(volatile uint8_t *sda_pin_reg, volatile uint8_t *sda_ddr, volatile uint8_t *sda_port, uint8_t sda_pin,
         volatile uint8_t *scl_pin_reg, volatile uint8_t *scl_ddr, volatile uint8_t *scl_port, uint8_t scl_pin)
    : SDA_PIN_REG(sda_pin_reg), SDA_DDR(sda_ddr), SDA_PORT(sda_port), SDA_PIN(sda_pin),
      SCL_PIN_REG(scl_pin_reg), SCL_DDR(scl_ddr), SCL_PORT(scl_port), SCL_PIN(scl_pin) 
{
    // Ensure pins are released (inputs) and enable pull-ups
    (*SDA_DDR) &= ~(1 << SDA_PIN); // input
    (*SDA_PORT) |= (1 << SDA_PIN); // enable pull-up
    (*SCL_DDR) &= ~(1 << SCL_PIN); // input
    (*SCL_PORT) |= (1 << SCL_PIN); // enable pull-up
}


// ---------- Public API ----------
void I2C::setDelay(int microseconds) {
    I2C_DELAY_US = microseconds > 0 ? microseconds : 1;
}

bool I2C::writeMessage(uint8_t address, const uint8_t *data, unsigned int length) {
    if (!startCondition()) {
        return false;
    }

    if (!writeByte(address << 1)) { // Write mode
        stopCondition();
        return false; // No ACK from slave
    }
    for (unsigned int i = 0; i < length; i++) {
        if (!writeByte(data[i])) {
            stopCondition();
            return false; // No ACK from slave
        }
    }
    stopCondition();
    return true;
}

bool I2C::readMessage(uint8_t address, uint8_t *data, unsigned int length) {
    if (!startCondition()) {
        return false;
    }

    if (!writeByte((address << 1) | 0x01)) { // Read mode
        stopCondition();
        return false; // No ACK from slave
    }
    for (unsigned int i = 0; i < length; i++) {
        bool ack = (i < length - 1); // ACK all but last byte
        if (!readByte(data[i], ack)) {
            stopCondition();
            return false; // Read error
        }
    }
    stopCondition();
    return true;
}

// ---------- Low-level pin control ----------
void I2C::pull_scl_low() {
    (*SCL_DDR) |= (1 << SCL_PIN); // Set SCL as output
    (*SCL_PORT) &= ~(1 << SCL_PIN); // Drive SCL low
}

void I2C::release_scl() {
    (*SCL_DDR) &= ~(1 << SCL_PIN); // Set SCL as input
    (*SCL_PORT) |= (1 << SCL_PIN); // Enable pull-up resistor on SCL
}

void I2C::pull_sda_low() {
    (*SDA_DDR) |= (1 << SDA_PIN); // Set SDA as output
    (*SDA_PORT) &= ~(1 << SDA_PIN); // Drive SDA low
}

void I2C::release_sda() {
    (*SDA_DDR) &= ~(1 << SDA_PIN); // Set SDA as input
    (*SDA_PORT) |= (1 << SDA_PIN); // Enable pull-up resistor on SDA
}

// Corrected read functions — read from PINx register and mask bit
bool I2C::read_scl() {
    return ( (*SCL_PIN_REG) & (1 << SCL_PIN) ) != 0;
}

bool I2C::read_sda() {
    return ( (*SDA_PIN_REG) & (1 << SDA_PIN) ) != 0;
}

inline void I2C::delay() {
    for (int i = 0; i < I2C_DELAY_US; ++i) {
        _delay_us(1);
    }
}

// ---------- Byte-level protocols ----------
bool I2C::writeByte(uint8_t data) {
    for (int i = 0; i < 8; i++) {
        pull_scl_low(); // Clock low phase
        delay();

        const bool bit = (data & 0x80) != 0;
        if (bit) {
            release_sda(); // Release SDA to send logic 1
        } else {
            pull_sda_low(); // Drive SDA low for logic 0
        }
        delay();

        release_scl(); // Allow line high; handle clock stretching
        if (!waitForSclHigh()) {
            pull_scl_low();
            return false;
        }
        delay();

        if (bit && !read_sda()) {
            arbitration_lost = true;
            release_sda();
            return false;
        }

        pull_scl_low();
        delay();

        data <<= 1;
    }

    // ACK/NACK bit
    pull_scl_low();
    release_sda(); // Release SDA for ACK/NACK
    delay();

    release_scl();
    if (!waitForSclHigh()) {
        pull_scl_low();
        return false;
    }
    delay();

    bool ack = !read_sda(); // ACK is low (0), NACK is high (1)

    pull_scl_low();
    delay();
    release_sda();

    return ack && !arbitration_lost;
}

bool I2C::readByte(uint8_t &data, bool ack) {
    data = 0;
    for (int i = 0; i < 8; i++) {
        data <<= 1;

        pull_scl_low();
        release_sda(); // Release SDA for reading
        delay();

        release_scl();
        if (!waitForSclHigh()) {
            pull_scl_low();
            return false;
        }
        delay();

        if (read_sda()) {
            data |= 0x01; // Read bit
        }
    }

    // Send ACK/NACK bit
    pull_scl_low();
    if (ack) {
        pull_sda_low(); // Send ACK (0)
    } else {
        release_sda(); // Send NACK (1)
    }
    delay();

    release_scl();
    if (!waitForSclHigh()) {
        pull_scl_low();
        release_sda();
        return false;
    }
    delay();

    pull_scl_low();
    release_sda();

    return true;
}

// ---------- Conditions ----------
bool I2C::waitForBusIdle(uint32_t timeoutUs) {
    release_sda();
    release_scl();

    while (timeoutUs--) {
        if (read_scl() && read_sda()) {
            return true;
        }
        _delay_us(1);
    }
    return false;
}

bool I2C::waitForSclHigh(uint32_t timeoutUs) {
    while (timeoutUs--) {
        if (read_scl()) {
            return true;
        }
        _delay_us(1);
    }
    return false;
}

bool I2C::startCondition() {
    arbitration_lost = false;

    if (!waitForBusIdle()) {
        return false;
    }

    pull_sda_low();
    delay();
    pull_scl_low();
    delay();
    return true;
}

void I2C::stopCondition() {
    if (arbitration_lost) {
        release_sda();
        release_scl();
        return;
    }

    pull_sda_low();
    delay();

    release_scl();
    waitForSclHigh();
    delay();

    release_sda();
    delay();
}
