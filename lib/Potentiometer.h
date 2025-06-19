#ifndef POTENTIOMETER_H
#define POTENTIOMETER_H

#include "General.h"

#define POTENTIOMETER_PIN 28 // Pino do potenciômetro

// Função para configurar o potenciômetro
void configure_potentiometer();

// Função para ler o valor do potenciômetro
uint16_t read_potentiometer();

#endif