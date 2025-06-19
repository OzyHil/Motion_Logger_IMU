#include "Potentiometer.h" // Inclusão do cabeçalho com definições do potenciômetro

// Função para configurar o potenciômetro utilizando ADC
void configure_potentiometer()
{
    // Inicializa o ADC no pino do potenciômetro (POTENTIOMETER_A) com valor de "wrap" especificado
    adc_gpio_init(POTENTIOMETER_PIN);
}

// Função para ler a posição do potenciômetro e retornar o valor
uint16_t read_potentiometer() {
    adc_select_input(2); // Seleciona o canal 0 do ADC para ler o valor do potenciômetro
    return adc_read(); // Lê o valor do potenciômetro e retorna
}