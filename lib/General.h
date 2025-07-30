#ifndef GENERAL_H
#define GENERAL_H

// Inclusão das bibliotecas padrão e específicas do hardware
#include <stdio.h>  // Biblioteca padrão para entrada/saída
#include <stdlib.h> // Biblioteca padrão para alocação de memória e conversões
#include <stdint.h> // Biblioteca padrão para tipos inteiros
#include <string.h> // Biblioteca manipular strings

#include <ctype.h>
#include <time.h>
#include <math.h> // Biblioteca para funções matemáticas

#include "pico/stdlib.h" // Biblioteca principal para o Raspberry Pi Pico
#include "pico/binary_info.h"

#include "hardware/gpio.h"   // Controle de GPIO (General Purpose Input/Output)
#include "hardware/pwm.h"    // Controle de PWM (Pulse Width Modulation)
#include "hardware/pio.h"    // Programação de E/S PIO (Programmable I/O)
#include "hardware/clocks.h" // Controle de clocks
#include "hardware/i2c.h"    // Comunicação I2C
#include "hardware/adc.h"    // Biblioteca da Raspberry Pi Pico para manipulação do conversor ADC
#include "hardware/rtc.h"

#include "pio_matrix.pio.h" // Programa específico para controle da matriz de LEDs

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

extern SemaphoreHandle_t xWaterPumpMutex, xWaterLevelMutex, xWaterLimitsMutex;

// Enum para definir os estados do sistema
typedef enum {
    SYSTEM_STATE_INITIALIZING, 
    SYSTEM_STATE_READY,         
    SYSTEM_STATE_RECORDING,     
    SYSTEM_STATE_SD_ACCESSING,   
    SYSTEM_STATE_ERROR           
} system_state_t;

#define BUFFER_SIZE 10

// Função para inicializar a configuração do sistema (clocks, I/O, etc.)
void init_system_config();

// Função para inicializar o PWM em um pino específico com um valor de wrap
void init_pwm(uint gpio, uint wrap);

void add_reading(uint new_value, uint readings[]);

#endif