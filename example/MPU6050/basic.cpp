#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>
#include <protocol_UART.h>
#include <device_MPU6050.h>

static UART debugUart(&PIND, &DDRD, &PORTD, PD1, &PIND, &DDRD, &PORTD, PD0, 9600UL);

static void debugPrintDecimal(uint32_t value) {
  char buf[11];
  uint8_t len = 0;
  do {
    buf[len++] = '0' + (value % 10);
    value /= 10;
  } while (value != 0);
  while (len--) {
    debugUart.sendByte(buf[len]);
  }
}

static void debugPrintFloat(float value) {
  if (value < 0.0f) {
    debugUart.sendByte('-');
    value = -value;
  }
  uint32_t scaled = (uint32_t)(value * 100.0f + 0.5f);
  debugPrintDecimal(scaled / 100);
  debugUart.sendByte('.');
  uint8_t fraction = scaled % 100;
  debugUart.sendByte('0' + (fraction / 10));
  debugUart.sendByte('0' + (fraction % 10));
}

int main(void) {
  sei();
  debugUart.begin();
  debugUart.sendString("Initializing MPU6050...\r\n");

  volatile uint8_t *sda_pin_reg = &PIND;
  volatile uint8_t *sda_ddr = &DDRD;
  volatile uint8_t *sda_port = &PORTD;
  uint8_t sda_pin = PD7;

  volatile uint8_t *scl_pin_reg = &PINB;
  volatile uint8_t *scl_ddr = &DDRB;
  volatile uint8_t *scl_port = &PORTB;
  uint8_t scl_pin = PB0;

  MPU6050 mpu(sda_pin_reg, sda_ddr, sda_port, sda_pin,
        scl_pin_reg, scl_ddr, scl_port, scl_pin);

  _delay_ms(1000);
  if (mpu.initialize()) {
    debugUart.sendString("MPU6050 initialized successfully.\r\n");
  } else {
    debugUart.sendString("Failed to initialize MPU6050.\r\n");
    while (1) {
      // Halt
    }
  }

  mpu.setAccelRange(0);

  while (1) {
    MPU6050::MPU6050_Data data;
    if (mpu.readAllSensors(data)) {
      debugUart.sendString("Accel (g): X=");
      debugPrintFloat(data.accel_x);
      debugUart.sendString(" Y=");
      debugPrintFloat(data.accel_y);
      debugUart.sendString(" Z=");
      debugPrintFloat(data.accel_z);
            debugUart.sendString(" | Gyro (°/s): X=");
            debugPrintFloat(data.gyro_x);
            debugUart.sendString(" Y=");
      debugPrintFloat(data.gyro_y);
      debugUart.sendString(" Z=");
      debugPrintFloat(data.gyro_z);
      debugUart.sendString(" | Temp (°C): ");
      debugPrintFloat(data.temperature);
      debugUart.sendString("\r\n");
    } else {
      debugUart.sendString("Failed to read sensor data.\r\n");
    }
    _delay_ms(200);
  }

  return 0;
}
