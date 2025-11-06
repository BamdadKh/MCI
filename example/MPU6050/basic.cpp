#include <device_MPU6050.h>
#include <Arduino.h>

int main() {
  init(); // Initialize Arduino core
  Serial.begin(9600);
  
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB
  }

  Serial.println("Initializing MPU6050...");
  // Initialize I2C pins on d7 and d8
  volatile uint8_t *sda_pin_reg = &PIND;
  volatile uint8_t *sda_ddr = &DDRD;
  volatile uint8_t *sda_port = &PORTD;
  uint8_t sda_pin = PD7; // D7

  volatile uint8_t *scl_pin_reg = &PINB;
  volatile uint8_t *scl_ddr = &DDRB;
  volatile uint8_t *scl_port = &PORTB;
  uint8_t scl_pin = PB0; // D8

  MPU6050 mpu(sda_pin_reg, sda_ddr, sda_port, sda_pin,
                scl_pin_reg, scl_ddr, scl_port, scl_pin);
  
  delay(1000);
  if (mpu.initialize()) {
    Serial.println("MPU6050 initialized successfully.");
  } else {
    Serial.println("Failed to initialize MPU6050.");
    while (1); // Halt
  }

  mpu.setAccelRange(0); // Set to ±2g

  while (1) {
    MPU6050::MPU6050_Data data;
    if (mpu.readAllSensors(data)) {
        Serial.print("Accel (g): X=");
        Serial.print(data.accel_x);
        Serial.print(" Y=");
        Serial.print(data.accel_y);
        Serial.print(" Z=");
        Serial.print(data.accel_z);
        Serial.print(" | Gyro (°/s): X=");
        Serial.print(data.gyro_x);
        Serial.print(" Y=");
        Serial.print(data.gyro_y);
        Serial.print(" Z=");
        Serial.print(data.gyro_z);
        Serial.print(" | Temp (°C): ");
        Serial.println(data.temperature);
    } else {
        Serial.println("Failed to read sensor data.");
    }
    delay(200);
  }
}
