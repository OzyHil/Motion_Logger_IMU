#ifndef MPU6050_H
#define MPU6050_H

#include "General.h"     // Definições gerais do sistema

// Endereço padrão do MPU6050
#define MPU6050_ADDR 0x68
#define I2C_PORT i2c0 // i2c0 usa pinos 0 e 1, i2c1 usa pinos 2 e 3
#define I2C_SDA 0     // SDA pode ser 0 ou 2
#define I2C_SCL 1     // SCL pode ser 1 ou 3

void configure_mpu6050();

// Lê aceleração (XYZ), giroscópio (XYZ) e temperatura bruta
void MPU6050_read_raw(int16_t accel[3], int16_t gyro[3]);

#endif
