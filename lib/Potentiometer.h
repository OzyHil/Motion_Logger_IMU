#ifndef POTENTIOMETER_H
#define POTENTIOMETER_H

#include "General.h"

#define POTENTIOMETER_PIN 28 // Pino do potenciômetro
#define MAX_ADC_VALUE 4095.0f // Valor máximo do ADC (12 bits)
#define MIN_ADC_VALUE 0.0f // Valor mínimo do potenciômetro

// Função para configurar o potenciômetro
void configure_potentiometer();

// Função para ler o valor do potenciômetro
float read_potentiometer();

// Função para mapear o valor lido do potenciômetro para um intervalo específico
float map_reading(float value, float in_min, float in_max, float out_min, float out_max);

#endif